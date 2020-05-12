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
        "vnic_name": "veth0",
        "port_id": 1
    },
    {
        "vnic_name": "veth1",
        "port_id": 2
    },
        {
        "vnic_name": "veth2",
        "port_id": 3
    },
        {
        "vnic_name": "veth3",
        "port_id": 4
    },
        {
        "vnic_name": "veth4",
        "port_id": 5
    },
    {
        "vnic_name": "veth5",
        "port_id": 6
    },
        {
        "vnic_name": "veth11",
        "port_id": 7
    },
    {
        "vnic_name": "veth12",
        "port_id": 8
    }
]

def get_click_instance_id(chain_id, nf_id, stage_index):
    return (chain_id << 16) + (nf_id << 8) + stage_index

def start_click_nf(nf, chain_id):
    global free_switch_port
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = get_click_instance_id(chain_id, nf.id, nf.stage_index)
    print 'click id give to element %d\n' % click_id
    start_command = "sudo ../click_nf/pattern/run.py -i %d ../click_nf/pattern/%s.pattern %s >logs/%s.log 2>&1 &" % (click_id, nf.click_file_name, nic_to_bind, nf.click_file_name)
    os.system(start_command)
    return switch_port['port_id']


def start_nfs(chain_head, chain_id):
    cur_nf = chain_head
    while cur_nf is not None:
        if cur_nf.offloadability == const.PARTIAL_OFFLOADABLE or cur_nf.offloadability == const.UN_OFFLOADABLE:
            cur_nf.running_port = start_click_nf(cur_nf, chain_id)
        else:
            cur_nf.running_port = -1
        cur_nf = cur_nf.next_nf

if __name__ == '__main__':
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = get_click_instance_id(0, 2, 2)
    start_command = "sudo ../click_nf/conf/run.py -i %d ../click_nf/conf/%s.pattern %s >logs/%s.log 2>&1 &" % (click_id, "nat-p4-encap", "veth1", "nat-p4-encap")
    os.system(start_command)
