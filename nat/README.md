# Network Address Translator

A NAT implemented with P4 and DPDK

## Commands
pktgen in mn.h1:
```bash
pktgen -l 0-1 -n 4 --file-prefix pktgen1 --no-pci --vdev 'eth_af_packet,iface=h1-eth0' -- -P -T -m"1.0"

set 0 count 1
set 0 src ip 10.0.1.1
set 0 dst ip 188.188.188.xxx
str
stp
```

xss-nat in mn.h2:
```bash
vi /etc/ld.so.conf add /root/nat/nat-dpdk #for libswitch.so, can remove later.
/sbin/ldconfig

cd /root/nat/nat-dpdk/build
./xss_nat -l 3 --file-prefix nat --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -p 1 --parse-ptype --config="(0,0,3)"
```

The first time you str, you will see pkt go through h2(i.e.,xss-nat). After that, pkts will be handled directly in bmv2 switch s2.

## Todo
1. query counters by [DirectCounter](https://p4.org/p4runtime/spec/v1.0.0/P4Runtime-Spec.html#sec-counter-directcounter)
2. According to counter, delete timeout rules in switch.
3. The division of responsibilities should be more clear. I mean, which logic should be done by NF and which logic should be done by the framework?