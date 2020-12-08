import argparse
import grpc

import os
import sys
# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import p4runtime_lib.helper
from p4runtime_lib.switch import ShutdownAllSwitchConnections
from p4runtime_lib.error_utils import printGrpcError
import p4runtime_lib.bmv2
import p4runtime_lib.simple_controller


class P4Controller(object):

    def __init__(self, p4info_file_path, bmv2_file_path):
        self.p4info_helper = p4runtime_lib.helper.P4InfoHelper(
            p4info_file_path)
        self.switch_connection = p4runtime_lib.bmv2.Bmv2SwitchConnection(
            name='s1',
            address='127.0.0.1:50051',
            device_id=0,
            proto_dump_file='../configurable_p4_dp/logs/s1-p4runtime-requests.txt'
        )
        self.switch_connection.MasterArbitrationUpdate()
        self.switch_connection.SetForwardingPipelineConfig(p4info=self.p4info_helper.p4info,
                                       bmv2_json_file_path=bmv2_file_path)

    def __get_prefix(self, stage_id):
        return "MyIngress.elementControl_%d" % (stage_id % 5)

    def insert_table_entries(self, entries):
        for entry in entries:
            self.showTableEntry(entry)
            self.switch_connection.WriteTableEntry(entry)

    def build_table_entry(self, entry_info):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name=entry_info["table_name"],
            match_fields=entry_info["match_fields"],
            action_name=entry_info["action_name"],
            action_params=entry_info.get("action_params", {}),
            priority=entry_info.get("priority")
        )
        return table_entry

    def showTableEntry(self, entry):
        table_name = self.p4info_helper.get_tables_name(entry.table_id)
        print table_name
        for m in entry.match:
            print "    %s: %r" % (self.p4info_helper.get_match_field_name(
                table_name, m.field_id), self.p4info_helper.get_match_field_value(m))
        action = entry.action.action
        action_name = self.p4info_helper.get_actions_name(action.action_id)
        print '        ->', action_name
        for p in action.params:
            print "            %s: %r" % (self.p4info_helper.get_action_param_name(
                action_name, p.param_id), p.value)
        print

    def insert_route(self, chain_id, chain_length, output_port):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name="MyIngress.forwardControl.chainId_exact",
            match_fields={
                "hdr.sfc.chainId": chain_id,
                "hdr.sfc.chainLength": chain_length,
            },
            action_name="MyIngress.forwardControl.set_output_port",
            action_params={
                "port": output_port
            }
        )
        self.insert_table_entries([table_entry])

    def insert_entry(self, chain_id, nf_id, stage_index, entry_info):
        # add prefix to table_name and action_name according to stage_id
        full_table_name = "%s.%s" % (self.__get_prefix(
            stage_index), entry_info['table_name'])
        full_action_name = "%s.%s" % (self.__get_prefix(
            stage_index), entry_info['action_name'])

        # each entry should match chain_id and stage_id
        match_fields = entry_info.get('match_fields', {})
        match_fields['hdr.sfc.chainId'] = chain_id
        match_fields['meta.curNfInstanceId'] = nf_id
        # hard-code because one nf implemented as one p4 element
        match_fields['meta.stageId'] = 0

        action_params = entry_info.get('action_params', {})

        priority = entry_info.get('priority')

        table_entry = self.network_switch_p4info_helper.buildTableEntry(
            table_name=full_table_name,
            match_fields=match_fields,
            action_name=full_action_name,
            action_params=action_params,
            priority=priority
        )
        self.network_switch_connection.WriteTableEntry(table_entry)
        print "New entry installed successfully...\n Table [%s]\n Action [%s]\n" % (
            full_table_name, full_action_name)

    def delete_entry(self, chain_id, nf_id, stage_index, entry_info):
        full_table_name = "%s.%s" % (self.__get_prefix(
            stage_index), entry_info['table_name'])
        match_fields = entry_info.get('match_fields', {})
        match_fields['hdr.sfc.chainId'] = chain_id
        match_fields['meta.curNfInstanceId'] = nf_id
        match_fields['meta.stageId'] = 0  # hard code
        table_entry = self.network_switch_p4info_helper.buildTableEntry(
            table_name=full_table_name,
            match_fields=match_fields,
            priority=entry_info['priority']
        )
        self.network_switch_connection.DeleteTableEntry(table_entry)
        print "Delete entry successfully...\n Table [%s]\n" % (full_table_name)

    def read_counter(self, stage_index, counter_info):
        full_counter_name = "%s.%s" % (self.__get_prefix(
            stage_index), counter_info['counter_name'])
        counter_id = self.network_switch_p4info_helper.get_counters_id(
            full_counter_name)
        for response in self.network_switch_connection.ReadCounters(counter_id, counter_info['counter_index']):
            for entity in response.entities:
                counter = entity.counter_entry
                # print "%s %d: %d packets (%d bytes))" % (
                #     counter_info['counter_name'], counter_info['counter_index'],
                #     counter.data.packet_count, counter.data.byte_count
                # )
                return {"packet_count": counter.data.packet_count, "byte_count": counter.data.byte_count}

    def config_pipeline(self, sfc):
        network_switch_entries = []

        network_switch_entries.extend(generate_element_control_rules(
            sfc.pre_host_chain_head, sfc.pre_host_chain_tail, sfc.id, self.network_switch_p4info_helper))
        network_switch_entries.extend(generate_element_control_rules(
            sfc.post_host_chain_head, sfc.post_host_chain_tail, sfc.id, self.network_switch_p4info_helper))

        network_switch_entries.extend(generate_forward_control_rules(
            sfc, self.network_switch_p4info_helper))

        for entry in network_switch_entries:
            self.network_switch_connection.WriteTableEntry(entry)

        print 'Network switch config successfully for chain %d.' % sfc.id
        self.config_rule_record[sfc.id] = network_switch_entries
    
    def delete_pipeline(self, chain_id):
        entries = self.config_rule_record.get(chain_id)
        if entries is not None:
            for entry in entries:
                self.network_switch_connection.DeleteTableEntry(entry)


if __name__ == '__main__':
    print "Hello from p4 controller module...."

    # test logic for insert
    # entry_info is used to mock message from element
    # entry_info's pattern should be element specific

    # chain_id = 0
    # stage_id = 0
    # entry_info = {
    #     "table_name": "monitor.Monitor_exact",
    #     "action_name": "monitor.count_packet",
    # }
    # p4Controller.insert_entry(chain_id, stage_id, entry_info)

    # chain_id = 0
    # stage_id = 1
    # entry_info = {
    #     "table_name": "firewall.Firewall_ternary",
    #     "match_fields": {
    #         "hdr.ipv4.dstAddr": (0x0a000303, 0xffffffff),
    #     },
    #     "action_name": "firewall.drop",
    #     "priority": 1
    # }
    # p4Controller.insert_entry(chain_id, stage_id, entry_info)

    # chain_id = 0
    # stage_id = 2
    # entry_info = {
    #     "table_name": "ipRewriter.IpRewriter_exact",
    #     "match_fields": {
    #         "hdr.ipv4.srcAddr": 0x0a000101,
    #         "hdr.ipv4.dstAddr": 0x0a000304,
    #         "hdr.ipv4.protocol": 0x06,
    #         "hdr.tcp_udp.srcPort": 0x162E,
    #         "hdr.tcp_udp.dstPort": 0x04d2,
    #     },
    #     "action_name": "ipRewriter.change_src_addr_and_port",
    #     "action_params": {
    #         "srcAddr": 0x0c0c0c0c,
    #         "srcPort": 0x2222,
    #     }
    # }
    # P4Controller.insert_entry(chain_id, stage_id, entry_info)

    # test logic for read counter
    # chain_id = 0
    # stage_id = 0
    # counter_info = {
    #     "counter_name": "monitor.total_packets",
    #     "counter_index": 0
    # }
    # p4Controller.read_counter(chain_id, stage_id, counter_info)

    # test logic for delete entry
    # chain_id = 0
    # stage_id = 1
    # entry_info = {
    #     "table_name": "firewall.Firewall_ternary",
    #     "match_fields": {
    #         "hdr.ipv4.dstAddr": (0x0a000303, 0xffffffff),
    #     },
    #     "priority": 1
    # }
    # p4Controller.delete_entry(chain_id, stage_id, entry_info)
