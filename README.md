# p4sfc

## Related Concepts

- protobuf
- grpc
- thrift: a framework for scalable cross-language services development

## Environment Settings（mininet+bmv2+p4c+PI/p4runtime+dpdk)

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

2. run *bmv2* as Switch in *mininet*. (sample command is following:)

   ```bash
   sudo python 1sw_demo.py --behavioral-exe <behavioral exe> --json <json file(compiled from p4)>
   ```

3. create Containernet instead of Mininet and add dockers instead of general hosts.

4. run mycontroller.py to add rules into p4switch by grpc interface.

#### TODO

- [DONE]~~controller~~
- [CONCEPT] diff ingress & egress in v1model;
- implement DPDK nfv in Docker;
- design state communicaiton between docker and p4switch;
- sfc workflow design
