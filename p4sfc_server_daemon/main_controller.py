from flask import Flask, request
import argparse
import time
from p4_controller import P4Controller
from control_rule_generator import SFC
from click_nf_runner import start_nfs

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
    return "Hello World!"


@app.route('/deploy_chain', methods=["POST"])
def deploy_chain():
    data = request.get_json()
    chain_id = data.get("chain_id")
    chain_length = data.get("chain_length")
    nfs = data.get("nfs")

    # construct sfc
    sfc = SFC(chain_id, chain_length, nfs)

    # start nfs in the server
    start_nfs(sfc.chain_head, chain_id)

    # config pkt process logic
    global p4_controller
    p4_controller.config_pipeline(sfc)
    return "OK"


@app.route("/delete_chain", methods=["POST"])
def delete_chain():
    data = request.get_json()
    chain_id = data.get("chain_id")
    p4_controller.delete_pipeline(chain_id)
    return str(int(time.time()*1000))


@app.route('/insert_entry', methods=["POST"])
def insert_entry():
    data = request.get_json()
    if data.get("magic") != "sonic-fnrg":
        return "Bad packet"
    instance_id = data.get("instance_id")
    print 'instance_id %d\n' % instance_id
    chain_id = get_chain_id(instance_id)
    nf_id = get_nf_id(instance_id)
    stage_index = get_stage_index(instance_id)

    entry_info = {
        "table_name": data.get("table_name"),
        "match_fields": data.get("match_fields"),
        "action_name": data.get("action_name"),
        "action_params": data.get("action_params"),
        "priority": data.get("priority")
    }
    print entry_info
    p4_controller.insert_entry(chain_id, nf_id, stage_index, entry_info)
    return "OK"


@app.route("/insert_route", methods=["POST"])
def insert_route():
    data = request.get_json()
    chain_id = data.get("chain_id")
    chain_length = data.get("chain_length")
    output_port = data.get("output_port")
    p4_controller.insert_route(chain_id, chain_length, output_port)
    return str(int(time.time()*1000))


@app.route('/delete_entry', methods=["POST"])
def delete_entry():
    data = request.get_json()
    instance_id = data.get("instance_id")
    chain_id = get_chain_id(instance_id)
    nf_id = get_nf_id(instance_id)
    stage_index = get_stage_index(instance_id)
    entry_info = {
        "table_name": data.get("table_name"),
        "match_fields": data.get("match_fields"),
        "priority": data.get("priority")
    }
    p4_controller.delete_entry(chain_id, nf_id, stage_index, entry_info)
    return "OK"


@app.route('/read_counter', methods=["GET"])
def read_counter():
    instance_id = int(request.args.get("instance_id").encode("utf-8"))
    counter_name = request.args.get("counter_name")
    counter_index = int(request.args.get("counter_index").encode("utf-8"))

    stage_index = get_stage_index(instance_id)
    counter_info = {
        "counter_name": counter_name,
        "counter_index": counter_index
    }
    return p4_controller.read_counter(stage_index, counter_info)


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
