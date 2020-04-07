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

    action port_forward(macAddr_t dstAddr, egressSpec_t port) {
        standard_metadata.egress_spec = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
    }

    table port_exact {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            port_forward;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = drop();
    }

    action nf() {
        if (hdr.foo.flag == 1)
            hdr.foo.flag = 0;
    }

    action change_src_addr_port(ip4Addr_t srcAddr, bit<16> srcPort) {
        hdr.ipv4.srcAddr = srcAddr;
        hdr.tcp_udp.srcPort = srcPort;
    }

    action change_dst_addr_port(ip4Addr_t dstAddr, bit<16> dstPort) {
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.dstPort = dstPort;
    }

    table nat_exact {
        key = {
            hdr.ipv4.srcAddr: exact;
            hdr.ipv4.dstAddr: exact;
            hdr.ipv4.protocol: exact;
            hdr.tcp_udp.srcPort: exact;
            hdr.tcp_udp.dstPort: exact;
        }
        actions = {
            change_src_addr_port;
            change_dst_addr_port;
            port_forward;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    apply {
        nat_exact.apply();
        port_exact.apply();
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
