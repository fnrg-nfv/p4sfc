/**
    P4 Implementation for each element supported by P4SFC
*/

#include "define.p4"

control IpRewriter(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action change_src_addr_and_port(ip4Addr_t srcAddr, transport_port_t srcPort) {
        hdr.ipv4.srcAddr = srcAddr;
        hdr.tcp_udp.srcPort = srcPort;
    }

    action change_dst_addr_and_port(ip4Addr_t dstAddr, transport_port_t dstPort) {
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.dstPort = dstPort;
    }

    table IpRewriter_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: exact;
            hdr.ipv4.dstAddr: exact;
            hdr.ipv4.protocol: exact;
            hdr.tcp_udp.srcPort: exact;
            hdr.tcp_udp.dstPort: exact;
        }
        actions = {
            change_src_addr_and_port;
            change_dst_addr_and_port;
            drop;
        }
        size = 1024;
        default_action = drop();
        // const entries = {
        //     (0, 0, 0x0a000101, 0x0a000303, 0x06, 0x162e, 0x04d2): change_src_addr_and_port(0x0a0a0a0a, 0x1a0a);
        //     (0, 1, 0x0a0a0a0a, 0x0a000303, 0x06, 0x1a0a, 0x04d2): change_src_addr_and_port(0x0b0b0b0b, 0x7777);
        //     (0, 2, 0x0b0b0b0b, 0x0a000303, 0x06, 0x7777, 0x04d2): change_src_addr_and_port(0x0c0c0c0c, 0x2222);
        //     (0, 3, 0x0c0c0c0c, 0x0a000303, 0x06, 0x2222, 0x04d2): change_src_addr_and_port(0x0d0d0d0d, 0x3333);
        //     (0, 4, 0x0d0d0d0d, 0x0a000303, 0x06, 0x3333, 0x04d2): change_src_addr_and_port(0x0e0e0e0e, 0x4444);
        //     (0, 5, 0x0e0e0e0e, 0x0a000303, 0x06, 0x4444, 0x04d2): change_src_addr_and_port(0x0f0f0f0f, 0x9999);
        // }
    }

    apply{
        IpRewriter_exact.apply();
    }
}