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
        //     (0, 2, 0x0a000101, 0x0a000304, 0x06, 0x162E, 0x04d2): change_src_addr_and_port(0x0c0c0c0c, 0x2222);
        // }
    }

    apply{
        IpRewriter_exact.apply();
    }
}


control Monitor(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    counter(128, CounterType.packets) total_packets;

    action count_packet() {
        bit<32> counter_index = ((bit<32>)hdr.sfc.chainId) << 16;
        counter_index = counter_index + (bit<32>) meta.stageId;
        total_packets.count(counter_index);
    }

    table Monitor_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.stageId: exact;
        }
        actions = {
            NoAction;
            count_packet;
        }
        default_action = NoAction();
        size = 1024;
        // const entries = {
        //     (0, 0): count_packet();
        // }
    }
    
    apply{
        Monitor_exact.apply();
    }
}

control Firewall(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    action drop() {
        mark_to_drop(standard_metadata);
    }

    table Firewall_ternary {
        key = {
            hdr.sfc.chainId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: ternary;
            hdr.ipv4.dstAddr: ternary;
            hdr.ipv4.protocol: ternary;
            hdr.tcp_udp.srcPort: ternary;
            hdr.tcp_udp.dstPort: ternary;
        }
        actions = {
            NoAction;
            drop;
        }
        default_action = NoAction();
        const entries = {
            (0, 1, 0x00000000 &&& 0x00000000, 0x0a000303 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
        }
    }
    
    apply{
        Firewall_ternary.apply();
    }
}