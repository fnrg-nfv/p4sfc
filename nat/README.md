# Network Address Translator

A NAT implemented with P4 and DPDK

## Commands
xss-nat:
```bash
./xss_nat -l 3 --file-prefix nat --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -p 1 --parse-ptype --config="(0,0,3,0,0)"
```

## Todo
Refactor nat-dpdk
