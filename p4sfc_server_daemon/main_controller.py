from flask import Flask, request
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

@app.route('/insert_entry', methods = ["POST"])
def insert_entry():
    data = request.get_json()
    instance_id = data.get("instance_id")
    chain_id = get_chain_id(instance_id)
    stage_id = get_stage_id(instance_id)
    print chain_id
    print stage_id

    entry_info = {
        "table_name": data.get("table_name"),
        "match_fields": data.get("match_fields"),
        "action_name": data.get("action_name"),
        "action_params": data.get("action_params"),
        "priority": data.get("priority")
    }
    print entry_info
    p4_controller.insert_entry(chain_id, stage_id, entry_info)
    return "OK"

@app.route('/delete_entry', methods = ["POST"])
def delete_entry():
    data = request.get_json()
    instance_id = data.get("instance_id")
    chain_id = get_chain_id(instance_id)
    stage_id = get_stage_id(instance_id)
    entry_info = {
        "table_name": data.get("table_name"),
        "match_fields": data.get("match_fields"),
        "priority": data.get("priority")
    }
    p4_controller.delete_entry(chain_id, stage_id, entry_info)
    return "OK"

@app.route('/read_counter', methods = ["GET"])
def read_counter():
    instance_id = int(request.args.get("instance_id").encode("utf-8"))
    counter_name = request.args.get("counter_name")
    counter_index = int(request.args.get("counter_index").encode("utf-8"))

    chain_id = get_chain_id(instance_id)
    stage_id = get_stage_id(instance_id)
    counter_info = {
        "counter_name": counter_name,
        "counter_index": counter_index
    }
    return p4_controller.read_counter(chain_id, stage_id, counter_info)



def main(p4info_file_path, server_port):
    global p4_controller
    p4_controller = P4Controller(p4info_file_path)
    print 'P4SFC server daemon init successfully...'
    app.run(host="0.0.0.0", port=server_port)



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC Server Daemon')
    parser.add_argument('--p4info', help='p4info proto in text format from p4c',
                        type=str, action="store", required=False,
                        default='../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    parser.add_argument('--server-port', help='port for RESTful API',
                        type=str, action="store", required=False, default=8090)
    args = parser.parse_args()
    main(args.p4info, args.server_port)
