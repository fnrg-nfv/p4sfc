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

    url = "http://10.149.252.27:8091/deploy_chain"
    payload = {
        "chain_desc": chain_desc,
        "route": route
    }
    headers = {
        'Content-Type': 'application/json'
    }

    chain_id = requests.request("POST", url, headers=headers,
                         data=json.dumps(payload))
    return int(chain_id.text)

def delete_chain_request(chain_id):
    url = "http://10.149.252.27:8091/delete_chain"
    payload = {
        "chain_id": chain_id
    }
    headers = {
        'Content-Type': 'application/json'
    }

    requests.request("POST", url, headers=headers,
                         data=json.dumps(payload))

if __name__ == '__main__':
    chain_id = deploy_chain_request(5)
    time.sleep(1)
    delete_chain_request(chain_id)
