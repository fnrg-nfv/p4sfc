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

free_switch_port = [
    {
        "vnic_name": "veth10",
        "switch_port": 10
    },
    {
        "vnic_name": "veth12",
        "switch_port": 11
    }
]

def start_click_nf(nf):
    global free_switch_port
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = nf.id + nf.stage_index
    return switch_port['switch_port']

def start_nfs(nfs):
    cur_nf = nfs
    while cur_nf is not None:
        if cur_nf.offloadability == const.PARTIAL_OFFLOADABLE or cur_nf.offloadability == const.UN_OFFLOADABLE:
            cur_nf.running_port = start_click_nf(cur_nf)
        else:
            cur_nf.running_port = -1
        cur_nf = cur_nf.next_nf
