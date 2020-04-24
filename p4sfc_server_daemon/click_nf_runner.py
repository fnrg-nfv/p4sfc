import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import const


free_switch_port = [
    {
        "vnic_name": "veth10",
        "port_id": 10
    },
    {
        "vnic_name": "veth12",
        "port_id": 11
    }
]

def get_click_instance_id(chain_id, nf_id, stage_index):
    return (chain_id << 16) + (nf_id << 8) + stage_index

def start_click_nf(nf, chain_id):
    global free_switch_port
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = get_click_instance_id(chain_id, nf.id, nf.stage_index)
    return switch_port['port_id']


def start_nfs(chain_head, chain_id):
    cur_nf = chain_head
    while cur_nf is not None:
        if cur_nf.offloadability == const.PARTIAL_OFFLOADABLE or cur_nf.offloadability == const.UN_OFFLOADABLE:
            cur_nf.running_port = start_click_nf(cur_nf, chain_id)
        else:
            cur_nf.running_port = -1
        cur_nf = cur_nf.next_nf
