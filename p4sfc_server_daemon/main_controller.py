from flask import Flask
import argparse
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

@app.route('/test')
def test():
    return "Hello World!"

@app.route('/insert')
def insert_new_entry(instance_id, entry_info):
    chain_id = get_chain_id(instance_id)
    stage_id = get_stage_id(instance_id)
    element = instance_id_to_element[instance_id]
    p4_controller.insert_entry(chain_id, stage_id, element, entry_info)

def main(p4info_file_path, server_port):
    global p4_controller
    p4_controller = P4Controller(p4info_file_path)
    print 'P4SFC server daemon init successfully...'
    # app.run(host="0.0.0.0", port=server_port)

    # test logic
    instance_id = (0 << 16) + 2
    global instance_id_to_element
    instance_id_to_element[instance_id] = p4sfc_element_utils.IPRewriter

    entry_info = {
        "table_name": "IpRewriter_exact",
        "match_fields": {
            "src_addr": 0x0a000101,
            "dst_addr": 0x0a000304,
            "protocol": 0x06,
            "src_port": 0x162E,
            "dst_port": 0x04d2,
        },
        "action_name": "change_src_addr_and_port",
        "action_params": {
            "src_addr": 0x0c0c0c0c,
            "src_port": 0x2222,
        }
    }
    insert_new_entry(instance_id, entry_info)

    instance_id = (0 << 16) + 0
    instance_id_to_element[instance_id] = p4sfc_element_utils.Monitor

    entry_info = {
        "table_name": "Monitor_exact",
        "action_name": "count_packet",
    }
    insert_new_entry(instance_id, entry_info)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC Server Daemon')
    parser.add_argument('--p4info', help='p4info proto in text format from p4c',
                        type=str, action="store", required=False,
                        default='../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    parser.add_argument('--server-port', help='port for RESTful API', type=str, action="store", required=False, default=8090)
    args = parser.parse_args()
    main(args.p4info, args.server_port)
