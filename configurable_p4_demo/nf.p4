/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "include/headers.p4"
#include "include/metadata.p4"
#include "include/parser.p4"
#include "include/checksum.p4"
#include "include/deparser.p4"

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }


    action set_stage_params(macAddr_t dstAddr, egressSpec_t port) {
        standard_metadata.egress_spec = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
    }

    table hdr_ternary {
        key = {
            hdr.sfc.chainId: exact;
            // hdr.ethernet.dstAddr: ternary;
            // hdr.ethernet.srcAddr: ternary;
            // hdr.ipv4.ihl: ternary;
            // hdr.ipv4.diffserv: ternary;
            // hdr.ipv4.totalLen: ternary;
            // hdr.ipv4.identification: ternary;
            // hdr.ipv4.flags: ternary;
            // hdr.ipv4.fragOffset: ternary;
            // hdr.ipv4.ttl: ternary;
            // hdr.ipv4.protocol: ternary;
            // hdr.ipv4.srcAddr: ternary;
            hdr.ipv4.dstAddr: ternary;
            hdr.tcp_udp.srcPort: ternary;
            // hdr.tcp_udp.dstPort: ternary;
        }
        actions = {
            set_stage_params;
            drop;
            NoAction;
        }
        // size = 1024;
        default_action = drop();
        const entries = {
            // !important value &&& mask
            (1, 0x0a000300 &&& 0xffffff00, 0x0000 &&& 0x0000): set_stage_params(0x123456123456, 2);
        }
    }

    // ,
    // {
    //   "table": "MyIngress.hdr_ternary",
    //   "match": {
    //         "hdr.sfc.chainId": 1,
    //         "hdr.ethernet.dstAddr": "0xadf" & "0xd",
    //         "hdr.ipv4.srcAddr": ternary,
    //         "hdr.ipv4.dstAddr": ternary
    //   },
    //   "action_name": "MyIngress.port_forward",
    //   "action_params": {
    //     "dstAddr": "08:00:00:00:03:00",
    //     "port": 2
    //   }
    // }

    // action nf() {
    //     if (hdr.foo.flag == 1)
    //         hdr.foo.flag = 0;
    // }

    // action change_src_addr_port(ip4Addr_t srcAddr, bit<16> srcPort) {
    //     hdr.ipv4.srcAddr = srcAddr;
    //     hdr.tcp_udp.srcPort = srcPort;
    // }

    // action change_dst_addr_port(ip4Addr_t dstAddr, bit<16> dstPort) {
    //     hdr.ipv4.dstAddr = dstAddr;
    //     hdr.tcp_udp.dstPort = dstPort;
    // }

    // table nat_exact {
    //     key = {
    //         hdr.ipv4.srcAddr: exact;
    //         hdr.ipv4.dstAddr: exact;
    //         hdr.ipv4.protocol: exact;
    //         hdr.tcp_udp.srcPort: exact;
    //         hdr.tcp_udp.dstPort: exact;
    //     }
    //     actions = {
    //         change_src_addr_port;
    //         change_dst_addr_port;
    //         port_forward;
    //         NoAction;
    //     }
    //     size = 1024;
    //     default_action = NoAction();
    // }

    apply {
        hdr_ternary.apply();
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
