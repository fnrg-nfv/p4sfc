import random
import requests
import json
import time

servers = ["s1", "s2", "s3"]
# servers = ["s3"]
nfs = [
    {
        "name": "Monitor"
    },
    {
        "name": "Firewall"
    },
    {
        "name": "IPRewriter",
        "click_file_name": "nat-p4-encap"
    }
]


def deploy_chain_request(chain_length):
    global servers
    global nfs
    route = ["ingress"]
    chain_desc = []
    server_index = 0
    for i in range(chain_length):
        server_index = random.randint(server_index, len(servers) - 1)
        nf_index = random.randint(0, len(nfs) - 1)
        nf = nfs[nf_index]
        nf_desc = {}
        nf_desc["name"] = nf["name"]
        if nf.get("click_file_name") != None:
            nf_desc["click_file_name"] = nf["click_file_name"]
        nf_desc["location"] = servers[server_index]
        chain_desc.append(nf_desc)
        if route[-1] != nf_desc["location"]:
            route.append(nf_desc["location"])
    route.append("egress")

    url = "http://10.149.252.24:8091/deploy_chain"
    payload = {
        "chain_desc": chain_desc,
        "route": route
    }
    headers = {
        'Content-Type': 'application/json'
    }

    print payload
    response = requests.request("POST", url, headers=headers,
                                data=json.dumps(payload))
    return json.loads(response.text)


def delete_chain_request(chain_id):
    url = "http://10.149.252.24:8091/delete_chain"
    payload = {
        "chain_id": chain_id
    }
    headers = {
        'Content-Type': 'application/json'
    }

    response = requests.request("POST", url, headers=headers,
                                data=json.dumps(payload))
    return json.loads(response.text)


def eval_configuration_time(times, chain_length):
    total_deploy_time = 0
    total_delete_time = 0
    for i in range(times):
        response = deploy_chain_request(chain_length)
        total_deploy_time = total_deploy_time + \
            (response["complete_time"] - response["receive_time"])
        time.sleep(1)
        response = delete_chain_request(response["chain_id"])
        total_delete_time = total_delete_time + \
            (response["complete_time"] - response["receive_time"])

    print "Running time: %d.  Chain_length: %d.\n  Avg deploy time: %d ms.\n  Avg delete time: %d ms.\n" % (
        times, chain_length, total_deploy_time / times, total_delete_time / times)


if __name__ == '__main__':
    eval_configuration_time(1, 10)