from flask import Flask
import argparse
import os
import sys
from p4_controller import P4Controller

app = Flask(__name__)
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
    p4_controller.insert_entry(chain_id, stage_id, entry_info)


def main(p4info_file_path, server_port):
    global p4_controller
    p4_controller = P4Controller(p4info_file_path)
    print 'P4SFC server daemon init successfully...'
    # app.run(host="0.0.0.0", port=server_port)

    # test logic
    instance_id = (0 << 16) + 2
    # mock message from element
    # pattern should be element specific!!!
    entry_info = {
        "table_name": "ipRewriter.IpRewriter_exact",
        "match_fields": {
            "hdr.ipv4.srcAddr": 0x0a000101,
            "hdr.ipv4.dstAddr": 0x0a000304,
            "hdr.ipv4.protocol": 0x06,
            "hdr.tcp_udp.srcPort": 0x162E,
            "hdr.tcp_udp.dstPort": 0x04d2,
        },
        "action_name": "ipRewriter.change_src_addr_and_port",
        "action_params": {
            "srcAddr": 0x0c0c0c0c,
            "srcPort": 0x2222,
        }
    }
    insert_new_entry(instance_id, entry_info)

    instance_id = (0 << 16) + 0
    entry_info = {
        "table_name": "monitor.Monitor_exact",
        "action_name": "monitor.count_packet",
    }
    insert_new_entry(instance_id, entry_info)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC Server Daemon')
    parser.add_argument('--p4info', help='p4info proto in text format from p4c',
                        type=str, action="store", required=False,
                        default='../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    parser.add_argument('--server-port', help='port for RESTful API',
                        type=str, action="store", required=False, default=8090)
    args = parser.parse_args()
    main(args.p4info, args.server_port)
