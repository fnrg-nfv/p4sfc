/*************************************************************************
*********************** E L E M E N T S  *********************************
*************************************************************************/

control IPRewriter(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action change_src_addr_and_port(ip4Addr_t srcAddr, bit<16> srcPort) {
        hdr.ipv4.srcAddr = srcAddr;
        hdr.tcp_udp.srcPort = srcPort;
    }

    action change_dst_addr_and_port(ip4Addr_t dstAddr, bit<16> dstPort) {
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.dstPort = dstPort;
    }

    table IPRewriter_exact {
        key = {
            hdr.sfc.chainId: exact;
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
        // size = 1024;
        default_action = drop();
        const entries = {
            (1, 0x0a000101, 0x0a000303, 0x06, 0x162e, 0x04d2): change_src_addr_and_port(0x0a0a0a0a, 0x1a0a);
        }
    }

    apply{
        IPRewriter_exact.apply();
    }
}