# p4sfc

## 背景知识

- protobuf
- grpc
- thrift: a framework for scalable cross-language services development

## 环境搭建（mininet+bmv2+p4c+PI/p4runtime+dpdk)

### First Try

1. [DONE] git clone bmv2，并按照bmv2的README进行安装
   - 测试时，test switch未通过，应该是bmv2自己代码的问题
2. [DONE] git clone p4c，并按照p4c的README进行安装
   - install protobuf

3. [DONE] install grpc. **branch**: tags/v1.17.2
4. git clone p4lang/tutorial
5. install PI [**failed**]
   - ~~systemrepo install **failed**~~

### Second Try

follow *user-bootstrap.sh*

- use the above protobuf.
  - rebuild protobuf
- ./behavioral-model/targets/simple_switch_grpc make **[failed]**.

### Third Try

Switch ubuntu version from 18.04 to **16.04**, and follow *user-bootstrap.sh*
**[succeed]**

## *P4*+*containernet*

### *P4*嵌入*mininet*原理

1. *p4c*: .p4 file => .json file

2. run *bmv2* in *mininet*

   ```bash
   sudo python 1sw_demo.py --behavioral-exe <behavioral exe> --json <json file(compiled from p4)>
   ```

#### TODO

- controller
- diff ingress & egress
