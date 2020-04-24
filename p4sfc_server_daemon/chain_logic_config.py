import argparse
import grpc
import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import p4runtime_lib.bmv2
from p4runtime_lib.error_utils import printGrpcError
from p4runtime_lib.switch import ShutdownAllSwitchConnections
import p4runtime_lib.helper

class _const:
    class ConstError(TypeError):
        pass

    def __setattr__(self, name, value):
        if name in self.__dict__:
            raise self.ConstError("Can't rebind const (%s)" % name)
        self.__dict__[name] = value


const = _const()

const.OFFLOADABLE = 0
const.PARTIAL_OFFLOADABLE = 1
const.UN_OFFLOADABLE = 2
const.ELEMENT_P4_ID = {
    "IPRewriter": 0,
    "Monitor": 1,
    "Firewall": 2,
    "VPN": 3
}
const.No_STAGE = 255


class NF:
    def __init__(self, nf_name, nf_id, offloadability, click_config, next_nf=None):
        self.name = nf_name
        self.id = nf_id
        self.offloadability = offloadability
        self.click_config = click_config
        self.next_nf = next_nf

    def set_next_nf(self, next_nf):
        if not isinstance(next_nf, NF):
            raise Exception("%s is not a NF." % next_nf)
        self.next_nf = next_nf


class SFC:
    def __init__(self, chain_id, chain_length, NFs):
        self.id = chain_id
        self.chain_length = chain_length
        self.chain_head = self.build_SFC(NFs)
        self.divide_chain()
        if self.pre_host_chain_head is not None:
            print "pre_host_chain head is %s" % self.pre_host_chain_head.name
        else:
            print "pre_host_chain is None"

        if self.host_chain_head is not None:
            print "host_chain head is %s" % self.host_chain_head.name
        else:
            print "host_chain is None"

        if self.post_host_chain_head is not None:
            print "post_host_chain head is %s" % self.post_host_chain_head.name
        else:
            print "post_host_chain is None"

        self.assign_stage_index()

    def build_SFC(self, NFs):
        cur_nf = None
        for nf in NFs[::-1]:
            cur_nf = NF(nf['name'], nf['id'], nf['offloadability'],
                         nf['click_config'], cur_nf)
        return cur_nf

    def divide_chain(self):
        """ Divide chain into three parts:
        pre_host_chain: NFs that before the first un-offloaedable NF in the chain
        host_chain: NFs between the first un-offloaedable NF and the last un-offloaedable NF in the chain
        post_host_chain: NFs that after the last un-offloaedable NF in the chain.
        """
        if self.chain_head.offloadability == const.OFFLOADABLE or self.chain_head.offloadability == const.PARTIAL_OFFLOADABLE:
            self.pre_host_chain_head = self.chain_head
        else:
            self.pre_host_chain_head = None
            self.host_chain_head = self.chain_head
        
        cur_nf = self.chain_head
        pre_nf = None
        while cur_nf is not None and (cur_nf.offloadability == const.OFFLOADABLE or cur_nf.offloadability == const.PARTIAL_OFFLOADABLE):
            pre_nf = cur_nf
            cur_nf = cur_nf.next_nf
        
        self.pre_host_chain_tail = pre_nf
        self.host_chain_head = cur_nf

        if cur_nf is None:
            self.host_chain_tail = None
            self.post_host_chain_head = None
        else:
            while cur_nf is not None:
                if cur_nf.offloadability == const.UN_OFFLOADABLE:
                    self.host_chain_tail = cur_nf
                cur_nf = cur_nf.next_nf
            self.post_host_chain_head = self.host_chain_tail.next_nf

        self.post_host_chain_tail = None
    
    def assign_stage_index(self):
        """Assign stage index to pre_host_chain and post_host_chain
        By using stage_index, we can now which stage to config when operating on table entry
        """
        if self.pre_host_chain_head is not None:
            stage_index = 0
            cur_nf = self.pre_host_chain_head
            while cur_nf != self.pre_host_chain_tail:
                cur_nf.stage_index = stage_index
                stage_index = stage_index + 1
                cur_nf = cur_nf.next_nf
            cur_nf.stage_index = stage_index
        
        if self.post_host_chain_head is not None:
            stage_index = 0
            cur_nf = self.pre_host_chain_head
            while cur_nf is not None:
                cur_nf.stage_index = stage_index
                stage_index = stage_index + 1
                cur_nf = cur_nf.next_nf


# To be tested
def generate_server_pkt_distribution_rules(head_nf, chain_id, p4info_helper):
    """Generate pkt distribution rules for bmv2 switch running in server.
    Correspongding p4 file is p4sfc_server_pkt_distribution.p4 
    For each partial-offloadable/un-offloadable NF, map  chainId+nfInstancId to out port
    """
    pkt_distribution_rules = []
    cur_nf = head_nf
    while cur_nf is not None:
        if cur_nf.offloadability == const.PARTIAL_OFFLOADABLE or cur_nf.offloadability == const.UN_OFFLOADABLE:
            table_entry = p4info_helper.buildTableEntry (
                table_name="MyIngress.nf_instance_id_exact",
                match_fields={
                    "hdr.sfc.chainId": chain_id,
                    "meta.curNfInstanceId": cur_nf.id
                },
                action_name="MyIngress.sent_to_nf_instance",
                action_params={
                    "port": cur_nf.running_port
                },
            )
            pkt_distribution_rules.append(table_entry)
        cur_nf = cur_nf.next_nf

    print "Generate pkt distribution rules successfully...\n  Rules sum: %d" % len(pkt_distribution_rules)
    return  pkt_distribution_rules

def get_prefix(stage_id):
    return "MyIngress.elementControl_%d" % (stage_id % 5)


def generate_element_control_rules(head_nf, tail_nf, chain_id, p4info_helper):
    """Generate the element control rules for p4 switch in the network
    Corresponding file is p4sfc_template.p4 and element_control.p4
    """
    if head_nf is None:
        return []
    element_control_rules = []
    cur_nf = head_nf
    curStage = 0
    while cur_nf is not None:
        nextStage = const.No_STAGE if cur_nf.next_nf is None else (curStage + 1) % 5
        table_entry = p4info_helper.buildTableEntry (
                table_name=get_prefix(curStage) + ".chainId_stageId_exact",
                match_fields={
                    "hdr.sfc.chainId": chain_id,
                    "meta.curNfInstanceId": cur_nf.id,
                    "meta.stageId": 0 # now assume every NF has only one stage! need to extent!
                },
                action_name= get_prefix(curStage) + ".set_control_data",
                action_params={
                    "elementId": const.ELEMENT_P4_ID[cur_nf.name],
                    "nextStage": nextStage,
                    "isNFcomplete": 1 # Constant. the reason is the saem as meta.stageId
                },
            )
        element_control_rules.append(table_entry)
        if cur_nf == tail_nf:
            break
        curStage = curStage + 1
        cur_nf = cur_nf.next_nf

    print "Generate element control rules successfully...\n  Rules sum: %d" % len(element_control_rules)
    return element_control_rules

def generate_stage_control_rules(head_nf, tail_nf, chain_id, p4info_helper):
    """Generate the stage control rules for p4 switch in the network
    Corresponding file is stage_control.p4
    Generate one stage control rule for the head-nf
    For each (partial-offloadable, offloadable) pair, generate one stage control rule
    """
    
    if head_nf is None:
        return []

    stage_control_rules = []
    cur_nf = head_nf
    table_entry = p4info_helper.buildTableEntry (
        table_name="MyIngress.stageControl.chainId_instanceId_exact",
        match_fields={
            "hdr.sfc.chainId": chain_id,
            "meta.curNfInstanceId": cur_nf.id,
        },
        action_name= "MyIngress.stageControl.set_stage",
        action_params={
            "stage_index": 0 # start from 0
        },
    )
    stage_control_rules.append(table_entry)

    if cur_nf == tail_nf:
        print "Generate stage control rules successfully...\n  Rules sum: %d" % len(stage_control_rules)
        return stage_control_rules

    pre_NF_offloadability = cur_nf.offloadability
    curStage = 1
    cur_nf = cur_nf.next_nf
    while cur_nf is not None:
        if pre_NF_offloadability == const.PARTIAL_OFFLOADABLE and cur_nf.offloadability == const.OFFLOADABLE:
            table_entry = p4info_helper.buildTableEntry (
                table_name="MyIngress.stageControl.chainId_instanceId_exact",
                match_fields={
                    "hdr.sfc.chainId": chain_id,
                    "meta.curNfInstanceId": cur_nf.id,
                },
                action_name= "MyIngress.stageControl.set_stage",
                action_params={
                    "stage_index": curStage % 5
                },
            )
            stage_control_rules.append(table_entry)

        if cur_nf == tail_nf:
            break

        pre_NF_offloadability = cur_nf.offloadability
        curStage = curStage + 1
        cur_nf = cur_nf.next_nf
    
    print "Generate stage control rules successfully...\n  Rules sum: %d" % len(stage_control_rules)
    return stage_control_rules

# To be tested
def generate_forward_control_rules(chain_head, chain_id, chain_length, p4info_helper):
    """Generate the forward rules for p4 switch in the network
    Only need one rule: when pre host chain has completed, send the pkt to server
    """
    cur_nf = chain_head
    while cur_nf is not None:
        if cur_nf.offloadability == const.OFFLOADABLE or cur_nf.offloadability == const.PARTIAL_OFFLOADABLE:
            chain_length = chain_length - 1
        else:
            break
        cur_nf = cur_nf.next_nf

    if cur_nf is not None:
        table_entry = p4info_helper.buildTableEntry (
            table_name="MyIngress.forwardControl.chainId_exact",
            match_fields={
                "hdr.sfc.chainId": chain_id,
                "hdr.sfc.chainLength": chain_length,
            },
            action_name= "MyIngress.stageControl.send_to_server"
        )

        return [table_entry]
    else:
        return [] # all NFs are offloaded, no rule need to be added.

if __name__ == '__main__':
    chain_id = 0
    chain_length = 3
    user_chain = [
        {
            "nf_name": "Monitor",
            "nf_id": 0,
            "offloadability": const.OFFLOADABLE,
            "click_config": {
                "haha": 666
            },
            "running_port": None
        },
        {
            "nf_name": "Firewall",
            "nf_id": 1,
            "offloadability": const.OFFLOADABLE,
            "click_config": {
                "haha": 666
            },
            "running_port": None
        },
        {
            "nf_name": "IPRewriter",
            "nf_id": 2,
            "offloadability": const.PARTIAL_OFFLOADABLE,
            "click_config": {
                "haha": 666
            },
            "running_port": 1
        }
    ]
    sfc = SFC(chain_id, chain_length, user_chain)

    p4info_helper = p4runtime_lib.helper.P4InfoHelper(
            '../configurable_p4_demo/build/p4sfc_server_pkt_distribution.p4.p4info.txt')
    generate_server_pkt_distribution_rules(sfc.chain_head, sfc.id, p4info_helper)

    
    
    # Below test is OK!

    p4info_helper = p4runtime_lib.helper.P4InfoHelper(
            '../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    switch_connection = p4runtime_lib.bmv2.Bmv2SwitchConnection(
            name='s2',
            address='127.0.0.1:50052',
            device_id=1,
            proto_dump_file='../configurable_p4_demo/logs/s2-p4runtime-requests.txt'
    )
    switch_connection.MasterArbitrationUpdate()

    rules = []
    rules.extend(generate_element_control_rules(sfc.pre_host_chain_head, sfc.pre_host_chain_tail, sfc.id, p4info_helper))
    rules.extend(generate_element_control_rules(sfc.post_host_chain_head, sfc.post_host_chain_tail, sfc.id, p4info_helper))
    
    rules.extend(generate_stage_control_rules(sfc.pre_host_chain_head, sfc.pre_host_chain_tail, sfc.id, p4info_helper))
    rules.extend(generate_stage_control_rules(sfc.post_host_chain_head, sfc.post_host_chain_tail, sfc.id, p4info_helper))

    for rule in rules:
        switch_connection.WriteTableEntry(rule)
    


