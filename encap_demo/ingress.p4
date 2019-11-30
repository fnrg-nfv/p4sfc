/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "header.p4h"

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

    action ingress_add_foo() {
        hdr.foo.setValid();
        hdr.foo.etherType = hdr.ethernet.etherType;
        hdr.foo.flag = 1;
        hdr.ethernet.etherType = TYPE_FOO;
    }
    
    apply {
        port_exact.apply();
        ingress_add_foo();
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
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.foo);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp_udp);
    }
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
