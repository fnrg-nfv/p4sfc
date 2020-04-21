/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "include/headers.p4"
#include "include/metadata.p4"
#include "include/checksum.p4"
#include "include/deparser.p4"

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select((bit<16>)hdr.ipv4.protocol) {
            PROTOCOL_TCP: parse_tcp_udp;
            PROTOCOL_UDP: parse_tcp_udp;
            default: accept;
        }
    }

    state parse_tcp_udp {
        packet.extract(hdr.tcp_udp);
        transition accept;
    }
}

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

    action add_p4sfc_headers() {
        hdr.sfc.setValid();
        hdr.sfc.chainId = 0;
        hdr.sfc.chainLength = 1;
        
        hdr.nfs[0].setValid();
        hdr.nfs[0].nfInstanceId = 0;
        hdr.nfs[0].isLast = 0;
    
        // hdr.nfs[1].setValid();
        // hdr.nfs[1].nfInstanceId = 1;
        // hdr.nfs[1].isLast = 0;
        
        // hdr.nfs[2].setValid();
        // hdr.nfs[2].nfInstanceId = 2;
        // hdr.nfs[2].isLast = 1;
    }
    
    apply {
        add_p4sfc_headers();
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
