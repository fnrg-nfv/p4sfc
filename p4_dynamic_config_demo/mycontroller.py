#!/usr/bin/env python2
import argparse
import grpc
import os
import sys
from time import sleep

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../utils/'))

import p4runtime_lib.bmv2
from p4runtime_lib.switch import ShutdownAllSwitchConnections
import p4runtime_lib.helper

def config_classifier(p4info_helper, sw):
    table_entry = p4info_helper.buildTableEntry(
        table_name="MyIngress.classifier_exact",
        match_fields={
            "hdr.ipv4.srcAddr": ("10.0.1.1"),
            "hdr.ipv4.dstAddr": ("10.0.2.2"),
            "hdr.ipv4.protocol": (6),
            "hdr.tcp_udp.srcPort": (5678),
            "hdr.tcp_udp.dstPort": (1234),
        },
        action_name="MyIngress.change_src_addr_and_port",
        action_params={
            "srcAddr": 0x13212121,
            "srcPort": 8888,
        }
    )
    sw.WriteTableEntry(table_entry)

def main(p4info_file_path, bmv2_file_path):
    p4info_helper = p4runtime_lib.helper.P4InfoHelper(p4info_file_path)

    s = p4runtime_lib.bmv2.Bmv2SwitchConnection(
        name='s1',
        address='127.0.0.1:50051',
        device_id=0,
        proto_dump_file='logs/s1-p4runtime-requests.txt'
    )

    s.MasterArbitrationUpdate()

    config_classifier(p4info_helper, s)



if __name__ == '__main__':
    main('./build/basic.p4.p4info.txt','./build/basic.json')