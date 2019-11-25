# p4sfc

## How to Run

Compile <u>test.p4</u> and run Containernet (<u>1sw_demo.py</u>):

```bash
make run
```

Open another terminal, run <u>mycontroller.py</u> as a controller to inject rules to bmv2.

```bash
python mycontroller.py
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



@

@

@



（以下为实验记录）

### Related Packages

- protobuf
- grpc
- thrift: a framework for scalable cross-language services development

## Environment Setting（containernet+bmv2+p4c+PI+docker+dpdk)

### First Try

1. [DONE] install bmv2, p4c, protobuf, grpc(tags/v1.17.2) according to their documents
   - test switch do not pass in bmv2
1. install PI [**failed**]

### Second Try

run *user-bootstrap.sh* in p4lang/tutorial directly.

- use protobuf built in **First Try**.
- ./behavioral-model/targets/simple_switch_grpc make **[failed]**.

### Third Try

Switch ubuntu version from 18.04 to **16.04**. Run *root-bootstrap.sh* and *user-bootstrap.sh*. **[succeed]**

## *P4*+*containernet*

1.  *p4c*: .p4 file => .json file & .txt file

2. run *bmv2* as Switch in *mininet*.

   ```bash
   sudo python 1sw_demo.py --behavioral-exe <behavioral exe> --json <json file(compiled from p4)>
   ```

3. create Containernet instead of Mininet and add dockers instead of general hosts.

4. run mycontroller.py to add rules into p4switch through grpc interface.

## *DPDK*+*Docker*

1. Make dpdk and build image

2. Run the image in containernet

   - bind `/dev/hugepages`

     ```python
     host = net.addDocker(<name>, ip=<ip>, mac=<mac>, dimage=<image>, volumes=['/dev/hugepages:/dev/hugepages:rw'])
     ```

3. Attach to the container, and bind the veth (created by containernet). Replace <veth> with the name of created virtual ethernet device.

    ```bash
    testpmd -l <core-list> -n <channel_number> --file-prefix vdev --no-pci --vdev 'eth_af_packet,iface=<veth>' -- -i
    ```

## TODO

- [CONCEPT] diff ingress & egress in v1model;
- implement pktgen in Docker;
- implement DPDK nfv and p4 in Docker;
- design **state** communicaiton between docker and p4switch;
- **sfc workflow** design.
