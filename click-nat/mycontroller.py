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

def printCounter(p4info_helper, sw, counter_name, index):
    for response in sw.ReadCounters(p4info_helper.get_counters_id(counter_name), index):
        for entity in response.entities:
            counter = entity.counter_entry
            print "%s %s %d: %d packets (%d bytes))" % (
                sw.name, counter_name, index,
                counter.data.packet_count, counter.data.byte_count
            )

def readTableRules(p4info_helper, sw):
    """
    Reads the table entries from all tables on the switch.

    :param p4info_helper: the P4Info helper
    :param sw: the switch connection
    """
    print '\n----- Reading tables rules for %s -----' % sw.name
    for response in sw.ReadTableEntries():
        for entity in response.entities:
            entry = entity.table_entry
            print entry

def main(p4info_file_path, bmv2_file_path):
    p4info_helper = p4runtime_lib.helper.P4InfoHelper(p4info_file_path)

    s = p4runtime_lib.bmv2.Bmv2SwitchConnection(
        name='s1',
        address='127.0.0.1:50051',
        device_id=0,
        proto_dump_file='logs/s1-p4runtime-requests.txt'
    )

    s.MasterArbitrationUpdate()

    # s.SetForwardingPipelineConfig(p4info=p4info_helper.p4info,
    #                               bmv2_json_file_path=bmv2_file_path)

    # readTableRules(p4info_helper, s)

    printCounter(p4info_helper, s, "MyIngress.myCounter", 0)

if __name__ == '__main__':
    main('./build/basic.p4.p4info.txt','./build/basic.json')