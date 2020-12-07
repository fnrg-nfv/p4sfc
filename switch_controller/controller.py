from flask import Flask, request
from p4_controller import P4Controller
import argparse
import time
import os
import sys
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '../utils/'))
import p4runtime_lib.helper

app = Flask(__name__)
p4_controller = None


def get_chain_id(instance_id):
    return instance_id >> 16


def get_nf_id(instance_id):
    return (instance_id >> 8) & 0x000000ff


def get_stage_index(instance_id):
    return instance_id & 0x000000ff


@app.route('/test')
def test():
    return "Hello from switch control plane controller~!"


@app.route('/config_pipeline', methods=["POST"])
def config_pipeline():
    data = request.get_json()
    operation_type = data.get("type")
    if operation_type == "insert":
        entries = []
        entry_infos = data.get("entry_infos")
        for entry_info in entry_infos:
            entries.append(p4_controller.build_table_entry(entry_info))
        p4_controller.insert_table_entries(entries)

    return "OK"

@app.route("/config_route", methods=["POST"])
def config_route():
    data = request.get_json()
    chain_id = data.get("chain_id")
    chain_length = data.get("chain_length")
    output_port = data.get("output_port")
    
    operation_type = data.get("type")
    if operation_type == "insert":
        p4_controller.insert_route(chain_id, chain_length, output_port)

    return str(int(time.time() * 1000))


def main(p4info_file_path, bmv2_file_path, server_port):
    global p4_controller
    p4_controller = P4Controller(p4info_file_path, bmv2_file_path)
    print 'P4SFC server daemon init successfully...'
    # Why Open debug will fail?????????????????????????????????????
    # app.run(host="0.0.0.0", port=server_port, debug=True)
    app.run(host="0.0.0.0", port=server_port)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC switch controller')
    parser.add_argument(
        '--p4info',
        help='p4info proto in text format from p4c',
        type=str,
        action="store",
        required=False,
        default='../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    parser.add_argument('--bmv2-json', help='BMv2 JSON file from p4c',
                        type=str, action="store", required=False,
                        default='../configurable_p4_demo/build/p4sfc_template.json')
    parser.add_argument('--server-port',
                        help='port for RESTful API',
                        type=str,
                        action="store",
                        required=False,
                        default=8090)
    args = parser.parse_args()
    main(args.p4info, args.bmv2_json, args.server_port)
