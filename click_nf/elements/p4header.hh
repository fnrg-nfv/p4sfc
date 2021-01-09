#ifndef P4HEADER_HH
#define P4HEADER_HH

#define P4H_IP_SADDR "hdr.ipv4.srcAddr"
#define P4H_IP_DADDR "hdr.ipv4.dstAddr"
#define P4H_IP_PROTO "hdr.ipv4.protocol"
#define P4H_IP_SPORT "hdr.tcp_udp.srcPort"
#define P4H_IP_DPORT "hdr.tcp_udp.dstPort"

#define P4_IPFILTER_TABLE_NAME "IpFilter_ternary"
#define P4_IPFILTER_ACTION_NAME "forward"
#define P4_IPFILTER_PARAM_PORT "port"

#define P4_IPRW_TABLE_NAME "IpRewriter_exact"
#define P4_IPRW_ACTION_NAME "rewrite"
#define P4_IPRW_PARAM_SA "srcAddr"
#define P4_IPRW_PARAM_DA "dstAddr"
#define P4_IPRW_PARAM_SP "srcPort"
#define P4_IPRW_PARAM_DP "dstPort"

#endif