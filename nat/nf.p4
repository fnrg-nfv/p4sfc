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

    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        standard_metadata.egress_spec = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            ipv4_forward;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = drop();
    }

    action change_src_addr_port(ip4Addr_t srcAddr, bit<16> srcPort) {
        hdr.ipv4.srcAddr = srcAddr;
        hdr.tcp_udp.srcPort = srcPort;
        hdr.foo.flag = 0;
        meta.recirculate = 1; // recirculate for sfc

    }

    action change_dst_addr_port(ip4Addr_t dstAddr, bit<16> dstPort) {
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.dstPort = dstPort;
        hdr.foo.flag = 0;
        meta.recirculate = 1; // recirculate for sfc

    }

    action send_to_server(egressSpec_t serverPort) {
        standard_metadata.egress_spec = serverPort;
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
            send_to_server;
            NoAction;
        }
        size = 1024;
        default_action = send_to_server(3);
    }

    apply {
        if (hdr.foo.flag == 1) {
            //need to nat first
            nat_exact.apply();
        }
        else {
            //nat has done. ipv4 forward
            ipv4_lpm.apply();
        }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  
        if (meta.recirculate == 1) {
            recirculate(standard_metadata);
        }
    }
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
