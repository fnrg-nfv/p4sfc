# p4sfc

## How to Run

Compile your p4 program and start Containernet topo
<!-- Compile <u>test.p4</u> and run Containernet (<u>1sw_demo.py</u>): -->

```bash
make run PROG_PREFIX=<your_p4_program_name> RUN_SCRIPT=<your_topo>.py
```

Open another terminal, run <u>your_controller.py</u> as a controller to inject rules to bmv2.

```bash
python <your_controller>.py --p4info ./bulid/<your_p4_program_name>.txt --bmv2-json ./build/<your_p4_program_name>.json
```

For each created container, attach to its bash and run: (replace <> with your own arguments)

```bash
<dpdk-app> -l <core-list> -n <channel_number> --file-prefix <prefix> --no-pci --vdev 'eth_af_packet,iface=<veth>' -- <app-args>
```

e.g.

```bash
./testpmd -l 1-2 -n 4 --file-prefix vdev --no-pci --vdev 'eth_af_packet,iface=h1-eth0' -- -i
```

```bash
./pktgen -l 0-2 -n 3 --file-prefix pktgen --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -P -m"[1:2].0"
```
