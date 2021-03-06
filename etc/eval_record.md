# p4sfc

A Service Function Chain(SFC) project led by Fudan Future Network Research Group(FNRG).

（以下为实验记录）

## Environment Setting（containernet+bmv2+p4c+PI+docker+dpdk)

### Related Packages

- protobuf
- grpc
- thrift: a framework for scalable cross-language services development

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

1. *p4c*: .p4 file => .json file & .txt file

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
- design **state** communicaiton between docker and p4switch;
- **sfc workflow** design.


## Evaluation

- service chain configuration time. (length from 3 to 10)
- recirculate overhead analysis
- 三类NF对比，click与offload的latency和throughput对比。
- chain latency and throughput comparasion with click and click-fast (e.g., click-dpdk)
- whole chain unoffloadable situation (NFP). 需要对比一下我们的方案和直接用click构建chain的方案开销。那个分发估计时延会比较大，但可以说我们兼容其他分发方式。
- ---
- Naive卸载策略和我们卸载策略对比。
- rule write time