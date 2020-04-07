/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

// ethernet type
const bit<16> TYPE_IPV4 = 0x800;


// ipv4 protocol
const bit<16> PROTOCOL_ICMP = 0x0001;
const bit<16> PROTOCOL_TCP = 0x0006;
const bit<16> PROTOCOL_UDP = 0x0007;


parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_sfc;
    }

    state parse_sfc {
        packet.extract(hdr.sfc);
        transition select(hdr.sfc.chainLength) {
            0: parse_ethernet;
            default: parse_nf;
        }
    }

    state parse_nf {
        packet.extract(hdr.nfs.next);
        transition select(hdr.nfs.last.isLast) {
            0: parse_nf;
            default: parse_ethernet;
        }
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