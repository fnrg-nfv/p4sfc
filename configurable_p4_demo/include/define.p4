#ifndef DEFINE_P4
#define DEFINE_P4

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

// support ethernet type
const bit<16> TYPE_IPV4 = 0x800;

// support ipv4 protocol type
const bit<16> PROTOCOL_ICMP = 0x0001;
const bit<16> PROTOCOL_TCP = 0x0006;
const bit<16> PROTOCOL_UDP = 0x0007;

#define MAX_SFC_LENGTH 10

// support elements
#define ELEMENT_IPREWRITER 1

#endif