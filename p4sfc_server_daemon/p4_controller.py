# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
import argparse
import grpc
import os
import sys
from time import sleep

sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import p4runtime_lib.helper
from p4runtime_lib.switch import ShutdownAllSwitchConnections
from p4runtime_lib.error_utils import printGrpcError
import p4runtime_lib.bmv2

sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 './include/'))
import p4sfc_element_utils

class P4Controller(object):

    def __init__(self, p4info_file_path):
        self.p4info_helper = p4runtime_lib.helper.P4InfoHelper(
            p4info_file_path)
        self.switch_connection = p4runtime_lib.bmv2.Bmv2SwitchConnection(
            name='s2',
            address='127.0.0.1:50052',
            device_id=1,
            proto_dump_file='../configurable_p4_demo/logs/s2-p4runtime-requests.txt'
        )
        self.switch_connection.MasterArbitrationUpdate()

    def __get_table_name_prefix(self, stege_id):
        return "MyIngress.elementControl_%d" % (stege_id % 5)

    def insert_entry(self, chain_id, stage_id, entry_info):
        entry_info['chain_id'] = chain_id
        entry_info['stage_id'] = stage_id
        entry_info['table_name'] = self.__get_table_name_prefix(
                stage_id) + ".ipRewriter.IpRewriter_exact"
        entry_info['action_name'] = self.__get_table_name_prefix(
                stage_id) + ".ipRewriter." + entry_info["action_name"]
        # table_entry = self.p4info_helper.buildTableEntry(
        #     table_name=self.__get_table_name_prefix(
        #         stage_id) + ".ipRewriter.IpRewriter_exact",
        #     match_fields={
        #         "hdr.sfc.chainId": chain_id,
        #         "meta.stageId": stage_id,
        #         "hdr.ipv4.srcAddr": entry_info["src_addr"],
        #         "hdr.ipv4.dstAddr": entry_info["dst_addr"],
        #         "hdr.ipv4.protocol": entry_info["protocol"],
        #         "hdr.tcp_udp.srcPort": entry_info["src_port"],
        #         "hdr.tcp_udp.dstPort": entry_info["dst_port"],
        #     },
        #     action_name=self.__get_table_name_prefix(
        #         stage_id) + ".ipRewriter." + entry_info["action_name"],
        #     action_params={
        #         "srcAddr": entry_info["action_params_src_addr"],
        #         "srcPort": entry_info["action_params_src_port"],
        #     }
        # )
        table_entry = p4sfc_element_utils.IPRewriter.build_IpRewriter_exact_entry(self.p4info_helper, entry_info)
        self.switch_connection.WriteTableEntry(table_entry)
        print "New entry installed successfully..."


if __name__ == '__main__':
    p4Controller = P4Controller('../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')
    entry_info = {"src_addr": 0x0c0c0c0c, "dst_addr": 0x0a000303, "protocol": 0x06, "src_port": 0x2222, "dst_port": 0x04d2,
                  "action_name": "change_src_addr_and_port", "action_params_src_addr": 0x0d0d0d0d, "action_params_src_port": 0x3333}
    p4Controller.insert_entry(0, 3, entry_info)
