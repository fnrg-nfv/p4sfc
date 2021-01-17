# Click with DPDK

## Run pktgen

```bash
# <pktgen.click> includes pktgen/*.click
sudo ~/fastclick/bin/click --dpdk -l 0-1 -n 4 -- <pktgen.click>
```

## Run distributor

```bash
sudo bash
~/fastclick/bin/click --dpdk -l 0-5 -n 4 --proc-type=primary -v -- distributor.click
```

## Run NFs


```bash
sudo bash
# <nf.click> includes nf/p4/*.click, nf/native/*.clcik,
#                     nf/lb.click and nf/pipe.click
~/fastclick/bin/click --dpdk -l 6 -n 4 --proc-type=secondary -v -- <nf.click>
```
