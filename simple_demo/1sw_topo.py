#!/usr/bin/env python2

from mininet.net import Mininet, Containernet
from mininet.node import Controller, Docker
from mininet.topo import Topo
from mininet.log import setLogLevel, info
from mininet.cli import CLI

import argparse
from time import sleep

import sys
import os
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             '../utils/'))

from p4_mininet import P4Switch

parser = argparse.ArgumentParser(description='Mininet demo')
parser.add_argument('--behavioral-exe', help='Path to behavioral executable',
                    type=str, action="store", required=True)
parser.add_argument(
    '--thrift-port',
    help='Thrift server port for table updates',
    type=int,
    action="store",
    default=9090)
parser.add_argument('--num-hosts', help='Number of hosts to connect to switch',
                    type=int, action="store", default=2)
parser.add_argument('--json', help='Path to JSON config file',
                    type=str, action="store", required=True)
parser.add_argument(
    '--pcap-dump',
    help='Dump packets on interfaces to pcap files',
    type=str,
    action="store",
    required=False,
    default=False)
parser.add_argument('-l', '--log-dir', help='Path to log files',
                    type=str, required=False)

args = parser.parse_args()


def main():
    num_hosts = args.num_hosts

    net = Containernet(controller=Controller,
                       switch=P4Switch)
    switch = net.addSwitch('s1',
                           sw_path=args.behavioral_exe,
                           json_path=args.json,
                           thrift_port=args.thrift_port,
                           pcap_dump=args.pcap_dump,
                           log_file=args.log_dir)

    print "%d" % net.get('s1').thrift_port

    sw_ip = ["10.0.%d.10" % n for n in range(num_hosts)]
    sw_mac = ["00:04:00:00:00:%02x" % n for n in range(num_hosts)]

    for h in range(num_hosts):
        host = net.addDocker('h%d' % (h + 1),
                             ip=sw_ip[h],
                             mac=sw_mac[h],
                             dimage="dpdk-pktgen:latest",
                             volumes=["/dev/hugepages:/dev/hugepages:rw"])
        net.addLink(host, switch)

    net.start()

    for n1 in range(num_hosts):
        h = net.get('h%d' % (n1 + 1))
        for n in range(num_hosts):
            if n is not n1:
                print "%d %s %s" % (n, sw_ip[n], sw_mac[n])
                h.setARP(sw_ip[n], sw_mac[n])
        # h.setDefaultRoute("dev eth0 via %s" % sw_ip[n1])

    print("Ready !")

    CLI(net)
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    main()
