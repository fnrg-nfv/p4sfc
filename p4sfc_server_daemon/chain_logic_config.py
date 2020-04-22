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
    "Firewall": 2
}
const.No_STAGE = 255


class NF:
    def __init__(self, nf_name, nf_id, offloadability, running_port, click_config, next_nf=None):
        self.name = nf_name
        self.id = nf_id
        self.offloadability = offloadability
        self.click_config = click_config
        self.running_port = running_port
        if next_nf is not None:
            self.next_nf = next_nf

    def set_next_nf(self, next_nf):
        if not isinstance(next_nf, NF):
            raise Exception("%s is not a NF." % next_nf)
        self.next_nf = next_nf


class SFC:
    def __init__(self, chain_id, NFs):
        self.id = chain_id
        self.chain_head = self.build_SFC(NFs)
        self.parse_chain()

    def build_SFC(self, NFs):
        cur_nf = None
        for nf_dict in NFs[::-1]:
            cur_nf = NF(nf_dict['nf_name'], nf_dict['nf_id'], nf_dict['offloadability'],
                        nf_dict['click_config'], nf_dict['running_port'], cur_nf)
        return cur_nf

    def parse_chain(self):
        """ Divide chain into three parts:
        pre_host_chain: NFs that before the first un-offloaedable NF in the chain
        host_chain: NFs between the first un-offloaedable NF and the last un-offloaedable NF in the chain
        post_host_chain: NFs that after the last un-offloaedable NF in the chain.
        """


def config_pre_host_chain(chain):
    """ Config the process logic of pre_host_chain
    Config stageControl and elementControl for chain
    For each partial-offloadable/un-offloadable NF, config p4sfc_server.p4
    For each (partial-offloadable, offloadable) pair, config stageControl
    """
    if chain is None:
        return
    pre_NF_offloadability = const.OFFLOADABLE


def config_host_chain(chain):
    """Config p4sfc_server.p4.
    Map each NF to a output port.
    """
    if chain is None:
        return


def config_post_host_chain(chain):
    """ Config the process logic of pre_host_chain
    Config stageControl and elementControl for chain
    For each partial-offloadable/un-offloadable NF, config p4sfc_server.p4
    For each (partial-offloadable, offloadable) pair, config stageControl
    """
    if chain is None:
        return
    pre_NF_offloadability = const.PARTIAL_OFFLOADABLE



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

    return  pkt_distribution_rules

def get_prefix(stage_id):
    return "MyIngress.elementControl_%d" % (stage_id % 5)


def generate_element_control_rules(head_nf, chain_id, p4info_helper):
    """Generate the element control rules for p4 switch in the network
    Corresponding file is p4sfc_template.p4 and element_control.p4
    """
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
        curStage = curStage + 1
        cur_nf = cur_nf.next_nf
    return element_control_rules

def generate_stage_control_rules(head_nf, chain_id, chain_type, p4info_helper):
    """Generate the stage control rules for p4 switch in the network
    Corresponding file is stage_control.p4
    Generate one stage control rule for the head-nf
    For each (partial-offloadable, offloadable) pair, generate one stage control rule
    """
    
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

        pre_NF_offloadability = cur_nf.offloadability
        curStage = curStage + 1
        cur_nf = cur_nf.next_nf
    
    return stage_control_rules

def generate_forward_control_rules():
    pass

