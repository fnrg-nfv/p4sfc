import argparse
import grpc
import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '../utils/'))
import p4runtime_lib.helper
from p4runtime_lib.switch import ShutdownAllSwitchConnections
from p4runtime_lib.error_utils import printGrpcError
import p4runtime_lib.bmv2
import const

element_p4_id = {"IPRewriter": 0, "Monitor": 1, "Firewall": 2, "VPN": 3}


class NF:
    def __init__(self,
                 nf_name,
                 nf_id,
                 offloadability,
                 click_file_name,
                 click_config,
                 next_nf=None):
        self.name = nf_name
        self.id = nf_id
        self.offloadability = offloadability
        self.click_file_name = click_file_name
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
                        nf.get('click_file_name'), nf.get('click_config'),
                        cur_nf)
        return cur_nf

    def divide_chain(self):
        """ Divide chain into three parts:
        pre_host_chain: NFs that before the first un-offloaedable NF in the chain
        host_chain: NFs between the first un-offloaedable NF and the last un-offloaedable NF in the chain
        post_host_chain: NFs that after the last un-offloaedable NF in the chain.
        For programming convenience, the interval is [ )
        """
        if self.chain_head.offloadability == const.OFFLOADABLE or self.chain_head.offloadability == const.PARTIAL_OFFLOADABLE:
            self.pre_host_chain_head = self.chain_head
        else:
            self.pre_host_chain_head = None

        # step to the host_chain head
        cur_nf = self.chain_head
        while cur_nf is not None and (
                cur_nf.offloadability == const.OFFLOADABLE
                or cur_nf.offloadability == const.PARTIAL_OFFLOADABLE):
            cur_nf = cur_nf.next_nf

        self.pre_host_chain_tail = cur_nf
        self.host_chain_head = cur_nf

        # set default value
        self.host_chain_tail = None
        self.post_host_chain_head = None
        self.post_host_chain_tail = None

        # step to the end of the chain to find
        # the post_host_chain_head
        while cur_nf is not None:
            if cur_nf.offloadability == const.UN_OFFLOADABLE:
                self.host_chain_tail = cur_nf.next_nf
                self.post_host_chain_head = cur_nf.next_nf
            cur_nf = cur_nf.next_nf

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

        if self.post_host_chain_head is not None:
            stage_index = 0
            cur_nf = self.pre_host_chain_head
            while cur_nf is not None:
                cur_nf.stage_index = stage_index
                stage_index = stage_index + 1
                cur_nf = cur_nf.next_nf


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
    while cur_nf is not None and cur_nf != tail_nf:
        nextStage = 255 if cur_nf.next_nf == tail_nf else (curStage + 1) % 5
        table_entry = p4info_helper.buildTableEntry(
            table_name=get_prefix(curStage) + ".chainId_stageId_exact",
            match_fields={
                "hdr.sfc.chainId": chain_id,
                "meta.curNfInstanceId": cur_nf.id,
                "meta.stageId":
                0  # now assume every NF has only one stage! need to extent!
            },
            action_name=get_prefix(curStage) + ".set_control_data",
            action_params={
                "elementId": element_p4_id[cur_nf.name],
                "nextStage": nextStage,
                "isNFcomplete":
                1  # Constant. the reason is the saem as meta.stageId
            },
        )
        element_control_rules.append(table_entry)
        if cur_nf == tail_nf:
            break
        curStage = curStage + 1
        cur_nf = cur_nf.next_nf

    print "Generate element control rules successfully...\n  Rules sum: %d" % len(
        element_control_rules)
    return element_control_rules


def generate_forward_control_rules(sfc, p4info_helper):
    """Generate the forward rules for p4 switch in the network
    For each possible position: [pre_host_head, pre_host_tail]
    & [post_host_head, post_host_tail], we generate one rule.
    """
    cur_nf = sfc.chain_head
    chain_length = sfc.chain_length
    chain_id = sfc.id
    forwarding_rules = []
    if sfc.pre_host_chain_head is not None:
        while cur_nf != sfc.pre_host_chain_tail:
            # build forward rule for each nf in the pre_server
            table_entry = p4info_helper.buildTableEntry(
                table_name="MyIngress.forwardControl.chainId_exact",
                match_fields={
                    "hdr.sfc.chainId": chain_id,
                    "hdr.sfc.chainLength": chain_length,
                },
                action_name="MyIngress.forwardControl.send_to_server")
            forwarding_rules.append(table_entry)
            cur_nf = cur_nf.next_nf
            chain_length = chain_length - 1

    # rule for the first nf in the server_chain
    table_entry = p4info_helper.buildTableEntry(
        table_name="MyIngress.forwardControl.chainId_exact",
        match_fields={
            "hdr.sfc.chainId": chain_id,
            "hdr.sfc.chainLength": chain_length,
        },
        action_name="MyIngress.forwardControl.send_to_server")
    forwarding_rules.append(table_entry)

    # step to the post_host_chain_head
    while cur_nf != sfc.post_host_chain_head:
        cur_nf = cur_nf.next_nf
        chain_length = chain_length - 1

    # generate the rules for post_host_chain if exist
    while cur_nf is not None:
        table_entry = p4info_helper.buildTableEntry(
            table_name="MyIngress.forwardControl.chainId_exact",
            match_fields={
                "hdr.sfc.chainId": chain_id,
                "hdr.sfc.chainLength": chain_length,
            },
            action_name="MyIngress.forwardControl.send_to_server")
        forwarding_rules.append(table_entry)
        cur_nf = cur_nf.next_nf
        chain_length = chain_length - 1

    return forwarding_rules


def showTableEntry(entry, p4info_helper):
    table_name = p4info_helper.get_tables_name(entry.table_id)
    print table_name
    for m in entry.match:
        print "    %s: %r" % (p4info_helper.get_match_field_name(
            table_name, m.field_id), p4info_helper.get_match_field_value(m))
    action = entry.action.action
    action_name = p4info_helper.get_actions_name(action.action_id)
    print '        ->', action_name
    for p in action.params:
        print "            %s: %r" % (p4info_helper.get_action_param_name(
            action_name, p.param_id), p.value)
    print

if __name__ == '__main__':
    chain_id = 0
    chain_length = 5
    user_chain = [{
        "name": "Monitor",
        "id": 0,
        "offloadability": const.OFFLOADABLE,
        "click_file_name": "monitor",
        "click_config": {
            "haha": 666
        }
    }, {
        "name": "Firewall",
        "id": 1,
        "offloadability": const.OFFLOADABLE,
        "click_file_name": "firewall",
        "click_config": {
            "haha": 666
        }
    }, {
        "name": "VPN",
        "id": 2,
        "offloadability": const.UN_OFFLOADABLE,
        "click_file_name": "vpn",
        "click_config": {
            "haha": 666
        }
    }, {
        "name": "VPN",
        "id": 2,
        "offloadability": const.UN_OFFLOADABLE,
        "click_file_name": "vpn",
        "click_config": {
            "haha": 666
        }
    },
    {
        "name": "IPRewriter",
        "id": 3,
        "offloadability": const.PARTIAL_OFFLOADABLE,
        "click_file_name": "iprewriter",
        "click_config": {
            "haha": 666
        }
    }]
    sfc = SFC(chain_id, chain_length, user_chain)

    p4info_helper = p4runtime_lib.helper.P4InfoHelper(
        '../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    # switch_connection = p4runtime_lib.bmv2.Bmv2SwitchConnection(
    #     name='s2',
    #     address='127.0.0.1:50052',
    #     device_id=1,
    #     proto_dump_file='../configurable_p4_demo/logs/s2-p4runtime-requests.txt'
    # )
    # switch_connection.MasterArbitrationUpdate()

    rules = []
    # rules.extend(
    #     generate_element_control_rules(sfc.pre_host_chain_head,
    #                                    sfc.pre_host_chain_tail, sfc.id,
    #                                    p4info_helper))
    # rules.extend(
    #     generate_element_control_rules(sfc.post_host_chain_head,
    #                                    sfc.post_host_chain_tail, sfc.id,
    #                                    p4info_helper))

    rules.extend(generate_forward_control_rules(sfc, p4info_helper))

    for rule in rules:
        # switch_connection.WriteTableEntry(rule)
        showTableEntry(rule, p4info_helper)
