from flask import Flask

import os
import sys
from p4_controller import P4Controller
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 './include/'))
import p4sfc_element_utils

app = Flask(__name__)
instance_id_to_element = {}
p4_controller = None

def get_chain_id(instance_id):
    return instance_id >> 16

def get_stage_id(instance_id):
    return instance_id & 0x0000ffff

def init(p4info_path):
    global p4_controller 
    p4_controller = P4Controller(p4info_path)
    
    # test logic
    # instance_id = (0 << 16) + 3
    # global instance_id_to_element
    # instance_id_to_element[instance_id] = p4sfc_element_utils.IPRewriter
    print 'P4SFC server daemon init succesfully...'


@app.route('/test')
def test():
    return "Hello World!"

@app.route('/insert')
def insert_new_entry(instance_id, entry_info):
    chain_id = get_chain_id(instance_id)
    stage_id = get_stage_id(instance_id)
    element = instance_id_to_element[instance_id]
    p4_controller.insert_entry(chain_id, stage_id, element, entry_info)


if __name__ == '__main__':
    # daemon start logic
    p4info_path = '../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt'
    init(p4info_path)
    app.run(host="0.0.0.0", port=8090, debug=True)

    # test logic
    # instance_id = (0 << 16) + 3
    # entry_info = {
    #     "table_name": "IpRewriter_exact",
    #     "match_fields": {
    #         "src_addr": 0x0c0c0c0c,
    #         "dst_addr": 0x0a000303,
    #         "protocol": 0x06,
    #         "src_port": 0x2222,
    #         "dst_port": 0x04d2,
    #     },
    #     "action_name": "change_src_addr_and_port",
    #     "action_params": {
    #         "src_addr": 0x0d0d0d0d,
    #         "src_port": 0x3333,
    #     }
    # }
    # insert_new_entry(instance_id, entry_info)
