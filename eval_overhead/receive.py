#!/usr/bin/env python
import sys
import struct
import os

from scapy.all import sniff, sendp, hexdump, get_if_list, get_if_hwaddr
from scapy.all import Packet, IPOption, bind_layers
from scapy.all import ShortField, IntField, LongField, BitField, FieldListField, FieldLenField
from scapy.all import IP, TCP, UDP, Raw, Ether
from scapy.layers.inet import _IPOption_HDR

def get_if():
    ifs=get_if_list()
    iface=None
    for i in get_if_list():
        if "eth0" in i:
            iface=i
            break
    if not iface:
        print "Cannot find eth0 interface"
        exit(1)
    return iface

class IPOption_MRI(IPOption):
    name = "MRI"
    option = 31
    fields_desc = [ _IPOption_HDR,
                    FieldLenField("length", None, fmt="B",
                                  length_of="swids",
                                  adjust=lambda pkt,l:l+4),
                    ShortField("count", 0),
                    FieldListField("swids",
                                   [],
                                   IntField("", 0),
                                   length_from=lambda pkt:pkt.count*4) ]

class sfc(Packet):
    fields_desc = [ BitField("chainId", 0, 16),
                    BitField("chainLength", 0, 16)]


class nfs(sfc):
   fields_desc = [ BitField("nfInstanceId", 0, 15),
                    BitField("isLast", 0, 1)]


bind_layers(sfc, nfs, chainLength=3)
bind_layers(nfs, nfs, isLast=0)
bind_layers(nfs, Ether, isLast=1)

def handle_pkt(pkt):
    print "got a packet"
    print pkt.show2()
    if TCP in pkt:
        pkt.show2()
    #    hexdump(pkt)
        sys.stdout.flush()

def main():
    ifaces = filter(lambda i: 's1-eth2' in i, os.listdir('/sys/class/net/'))
    iface = ifaces[0]
    print "sniffing on %s" % iface
    sys.stdout.flush()
    sniff(iface = iface,
          prn = lambda x: handle_pkt(x))

if __name__ == '__main__':
    main()