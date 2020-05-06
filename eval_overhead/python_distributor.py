import sys
import os
from scapy.all import sniff, sendp

out_iface = None
def handle_pkt(pkt):
    global out_iface
    sendp(pkt, iface=out_iface, verbose=False)


def main():
    ifaces = filter(lambda i: 'veth0' in i, os.listdir('/sys/class/net/'))
    global out_iface
    out_iface = ifaces[0]

    ifaces = filter(lambda i: 's1-eth2' in i, os.listdir('/sys/class/net/'))
    iface = ifaces[0]
    print "sniffing on %s" % iface
    sys.stdout.flush()
    sniff(iface = iface,
          prn = lambda x: handle_pkt(x))
    

if __name__ == '__main__':
    main()