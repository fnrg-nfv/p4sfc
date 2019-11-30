const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_FOO = 0xcdf;  // customed type

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header foo_t {
    bit<1>      flag;
    bit<15>     offset;
    bit<16>     etherType;
}

struct headers {
    ethernet_t   ethernet;
    foo_t        foo;
}

struct metadata {
    /* empty */
}
