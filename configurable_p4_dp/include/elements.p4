/**
    P4 Implementation for each element supported by P4SFC
*/

// partial offload element's default action is send_to_server
// full offload element's default action is element specific.
#include "define.p4"


// the default action for each element can be NoAction
// because the nextStage will be set as NO_STAGE in the
// element_complete_control logic as the isStageComplete is 0

control IpRewriter(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action rewrite(ip4Addr_t srcAddr, ip4Addr_t dstAddr, transport_port_t srcPort, transport_port_t dstPort) {
        meta.isStageComplete = 1;
        hdr.ipv4.srcAddr = srcAddr;
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.srcPort = srcPort;
        hdr.tcp_udp.dstPort = dstPort;
        
        rule_frequency.count();
    }

    table IpRewriter_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: exact;
            hdr.ipv4.dstAddr: exact;
            hdr.tcp_udp.srcPort: exact;
            hdr.tcp_udp.dstPort: exact;
        }
        actions = {
            p4sfcNoAction;
            rewrite;
        }
        default_action = p4sfcNoAction();
        counters=rule_frequency;
        // size = 1024;
        // const entries = {
            // (0, 0, 0, 0x0AA80001, 0x01010101, 0x162E, 0x04d2): rewrite(0x0a0a0a0a, 0x0b0b0b0b, 0x1111, 0x2222);
            // (0, 2, 2, 0x0a000101, 0x0a000303, 0x162E, 0x04d2): rewrite(0x0c0c0c0c, 0x0d0d0d0d, 0x3333, 0x4444);
        // }
    }

    apply{
        IpRewriter_exact.apply();
    }
}


control Monitor(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    // counter(128, CounterType.packets) total_packets;
    counter(128, CounterType.bytes) total_packets;
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action count_packet() {
        meta.isStageComplete = 1;
        bit<32> counter_index = ((bit<32>)hdr.sfc.chainId);
        // counter_index = counter_index + ((bit<32>) hdr.nfs[0].nfInstanceId << 8);
        // counter_index = counter_index + (bit<32>) meta.stageId;
        total_packets.count(counter_index);

        rule_frequency.count();        
    }

    table Monitor_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
        }
        actions = {
            p4sfcNoAction;
            count_packet;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        // size = 1024;
        const entries = {
            (0, 0, 0): count_packet();
        }
    }
    
    apply{
        Monitor_exact.apply();
    }
}

control Firewall(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    direct_counter(CounterType.packets_and_bytes) rule_frequency;
    
    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action drop() {
        meta.isStageComplete = 1;
        mark_to_drop(standard_metadata);

        rule_frequency.count();
    }

    table Firewall_ternary {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: ternary;
            hdr.ipv4.dstAddr: ternary;
            hdr.ipv4.protocol: ternary;
            hdr.tcp_udp.srcPort: ternary;
            hdr.tcp_udp.dstPort: ternary;
        }
        actions = {
            p4sfcNoAction;
            drop;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        // size = 1024;
        const entries = {
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a000301 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a0f0304 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0acd0304 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0abb0304 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0aaa0304 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a0003FF &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a0003AD &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a000312 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a000305 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a000306 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
            (0, 0, 0, 0x00000000 &&& 0x00000000, 0x0a000301 &&& 0xffffffff, 0x00 &&& 0x00, 0x0000 &&& 0x0000, 0x0000 &&& 0x0000): drop();
        }
    }
    
    apply{
        Firewall_ternary.apply();
    }
}

control Classifier(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action drop() {
        meta.isStageComplete = 1;
        mark_to_drop(standard_metadata);

        rule_frequency.count();
    }

    action set_next_stage(bit<8> nextStage) {
        meta.isStageComplete = 1;
        meta.nextStage = nextStage;

        rule_frequency.count();
    }

    table Classifier_ternary {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: ternary;
        }
        actions = {
            p4sfcNoAction;
            drop;
            set_next_stage;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        // size = 1024;
        const entries = {
            (0, 0, 0, 0x0AA80001 &&& 0xffffffff): set_next_stage(255);
        }
    }
    
    apply{
        Classifier_ternary.apply();
    }
}

control IpRoute(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action drop() {
        meta.isStageComplete = 1;
        mark_to_drop(standard_metadata);

        rule_frequency.count();
    }
    
    action set_output_port(egressSpec_t port) {
        meta.isStageComplete = 1;
        standard_metadata.egress_spec = port;

        rule_frequency.count();
    }

    table IpRoute_ternary {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.dstAddr: ternary;
        }
        actions = {
            p4sfcNoAction;
            drop;
            set_output_port;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        // size = 1024;
        const entries = {
            (0, 0, 0, 0x01010101 &&& 0xffffffff): set_output_port(132);
        }
    }
    
    apply{
        IpRoute_ternary.apply();
    }
}