# -*- coding:utf-8 -*-
from flask import Flask, request
import requests
import json
import time
import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import const


def addr2dec(addr):
    "将点分十进制IP地址转换成十进制整数"
    items = [int(x) for x in addr.split(".")]
    return sum([items[i] << [24, 16, 8, 0][i] for i in range(4)])


def dec2addr(dec):
    "将十进制整数IP转换成点分十进制的字符串IP地址"
    return ".".join([str(dec >> x & 0xff) for x in [24, 16, 8, 0]])

app = Flask(__name__)
chain_id = 0
server_addr = {
    "s1": "http://localhost:8090"
}
headers = {
    'Content-Type': 'application/json'
}
nf_offlodability = {
    "Monitor": const.OFFLOADABLE,
    "Firewall": const.OFFLOADABLE,
    "IPRewriter": const.PARTIAL_OFFLOADABLE
}



def parse_chain(chain_desc):
    """Parse user input chain
    assign id to each nf and group nf by location
    assgin offloadability to each nf
    """
    global nf_offlodability
    nf_id = 0
    cur_location = None
    cur_group = None
    nf_groups = {}
    for nf in chain_desc:
        # assign id first
        nf['id'] = nf_id
        nf_id = nf_id + 1

        # assgin offloadability
        nf['offloadability'] = nf_offlodability[nf['name']]
        # group by location
        location = nf['location']
        if location == cur_location:
            cur_group.append(nf)
        else:
            if cur_location is not None:
                nf_groups[cur_location] = cur_group
            cur_location = location
            cur_group = [nf]

    nf_groups[cur_location] = cur_group
    return nf_groups

def parse_route(chain_route, nf_groups, chain_id, chain_length):
    route_infos = {}
    for switch in  chain_route:
        if switch != "egress" and switch != "ingress":
            num_nfs =  len(nf_groups.get(switch)) if nf_groups.get(switch) != None else 0
            chain_length = chain_length - num_nfs    
            route_infos[switch] = {
                "chain_id": chain_id,
                "chain_length": chain_length,
                "output_port": 0 # 硬编码一下，本来应该根据拓扑决定
            }
    return route_infos




@app.route('/test')
def test():
    return "Hello from p4sfc ochestrator\n"


@app.route('/deploy_chain', methods=['POST'])
def deploy_chain():
    global chain_id
    global server_addr
    global headers

    print "Receive deploy request....\n  Chain_id: %d\n  Time: %d ms\n" % (chain_id, time.time()*1000)

    data = request.get_json()
    chain_desc = data.get("chain_desc")
    chain_length = len(chain_desc)
    nf_groups = parse_chain(chain_desc)

    for location, nfs in nf_groups.iteritems():
        url = server_addr[location] + "/deploy_chain"
        payload = {
            "chain_id": chain_id,
            "chain_length": chain_length,
            "nfs": nfs
        }
        chain_length = chain_length - len(nfs)
        requests.request("POST", url, headers=headers,
                         data=json.dumps(payload))

    chain_route = data.get("route")
    chain_length = len(chain_desc)
    route_infos = parse_route(chain_route, nf_groups, chain_id, chain_length)
    for switch, route_info in route_infos.iteritems():
        url = server_addr[switch] + "/insert_route"
        payload = {
            "chain_id": route_info["chain_id"],
            "chain_length": route_info["chain_length"],
            "output_port": route_info["output_port"]
        }
        requests.request("POST", url, headers=headers,
                         data=json.dumps(payload))

    chain_id = chain_id + 1
    return "OK"



if __name__ == '__main__':
    app.run(host="0.0.0.0", port='8091')
    {
        "chain_desc": [
            {
                "name": "Monitor",
                "click_config": {
                    "param1": "abc"
                },
                "location": "s1"
            },
            {
                "name": "Firewall",
                "click_config": {
                    "param1": "abc"
                },
                "location": "s1"
            },
            {
                "name": "IPRewriter",
                "click_config": {
                    "param1": "abc"
                },
                "location": "s1"
            }
        ],
        "route": [
            "ingress",
            "s1",
            "egress"
        ]
    }
