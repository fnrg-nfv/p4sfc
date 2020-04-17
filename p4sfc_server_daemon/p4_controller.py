import argparse
import grpc
import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import p4runtime_lib.bmv2
from p4runtime_lib.error_utils import printGrpcError
from p4runtime_lib.switch import ShutdownAllSwitchConnections
import p4runtime_lib.helper


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

    def __get_prefix(self, stege_id):
        return "MyIngress.elementControl_%d" % (stege_id % 5)

    def insert_entry(self, chain_id, stage_id, entry_info):
        # add prefix to table_name and action_name according to stage_id
        full_table_name = "%s.%s" % (self.__get_prefix(
            stage_id), entry_info['table_name'])
        full_action_name = "%s.%s" % (self.__get_prefix(
            stage_id), entry_info['action_name'])

        # each entry should match chain_id and stage_id
        match_fields = entry_info.get('match_fields', {})
        match_fields['hdr.sfc.chainId'] = chain_id
        match_fields['meta.stageId'] = stage_id

        action_params = entry_info.get('action_params', {})

        priority = entry_info.get('priority')

        table_entry = self.p4info_helper.buildTableEntry(
            table_name=full_table_name,
            match_fields=match_fields,
            action_name=full_action_name,
            action_params=action_params,
            priority=priority
        )
        self.switch_connection.WriteTableEntry(table_entry)
        print "New entry installed successfully...\n Table [%s]\n Action [%s]\n" % (
            full_table_name, full_action_name)
    
    def delete_entry(self, chain_id, stage_id, entry_info):
        full_table_name = "%s.%s" % (self.__get_prefix(
            stage_id), entry_info['table_name'])
        match_fields = entry_info.get('match_fields', {})
        match_fields['hdr.sfc.chainId'] = chain_id
        match_fields['meta.stageId'] = stage_id
        table_entry = self.p4info_helper.buildTableEntry(
            table_name=full_table_name,
            match_fields=match_fields,
            priority=entry_info['priority']
        )
        self.switch_connection.DeleteTableEntry(table_entry)
        print "Delete entry successfully...\n Table [%s]\n" % (full_table_name)
    
    def read_counter(self, chain_id, stage_id, counter_info):
        full_counter_name = "%s.%s" % (self.__get_prefix(stage_id), counter_info['counter_name'])
        counter_id = self.p4info_helper.get_counters_id(full_counter_name)
        for response in self.switch_connection.ReadCounters(counter_id, counter_info['counter_index']):
            for entity in response.entities:
                counter = entity.counter_entry
                # print "%s %d: %d packets (%d bytes))" % (
                #     counter_info['counter_name'], counter_info['counter_index'],
                #     counter.data.packet_count, counter.data.byte_count
                # )
                return {"packet_count": counter.data.packet_count, "byte_count": counter.data.byte_count}


if __name__ == '__main__':
    p4Controller = P4Controller(
        '../configurable_p4_demo/build/p4sfc_template.p4.p4info.txt')

    chain_id = 0
    stage_id = 1
    entry_info = {
        "table_name": "firewall.Firewall_ternary",
        "match_fields": {
            # "hdr.ipv4.srcAddr": (0x00000000, 0x00000000),
            "hdr.ipv4.dstAddr": (0x0a000303, 0xffffffff),
            # "hdr.ipv4.protocol": (0x00, 0x00),
            # "hdr.tcp_udp.srcPort": (0x0000, 0x0000),
            # "hdr.tcp_udp.dstPort": (0x0000, 0x0000)
        },
        "priority": 1
    }
    p4Controller.delete_entry(chain_id, stage_id, entry_info)
