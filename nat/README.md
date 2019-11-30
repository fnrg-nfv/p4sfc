# Network Address Translator

A NAT implemented with P4 and DPDK

## Commands
xss-nat:
```bash
./xss_nat -l 3 --file-prefix nat --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -p 1 --parse-ptype --config="(0,0,3)"
```

## Todo
1. NAT rule timeout detection
2. Parse user-defined protocol header
3. Insert nat rule to bmv2 through gprc
