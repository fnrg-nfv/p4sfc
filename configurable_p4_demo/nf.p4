/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "include/headers.p4"
#include "include/metadata.p4"
#include "include/parser.p4"
#include "include/checksum.p4"
#include "include/deparser.p4"
#include "include/element_control.p4"
#include "include/route_control.p4"

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    // action drop() {
    //     mark_to_drop(standard_metadata);
    // }


    // action set_stage_params(macAddr_t dstAddr, egressSpec_t port) {
    //     standard_metadata.egress_spec = port;
    //     hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
    //     hdr.ethernet.dstAddr = dstAddr;
    // }

    // table hdr_ternary {
    //     key = {
    //         hdr.sfc.chainId: exact;
    //         // hdr.ethernet.dstAddr: ternary;
    //         // hdr.ethernet.srcAddr: ternary;
    //         // hdr.ipv4.ihl: ternary;
    //         // hdr.ipv4.diffserv: ternary;
    //         // hdr.ipv4.totalLen: ternary;
    //         // hdr.ipv4.identification: ternary;
    //         // hdr.ipv4.flags: ternary;
    //         // hdr.ipv4.fragOffset: ternary;
    //         // hdr.ipv4.ttl: ternary;
    //         // hdr.ipv4.protocol: ternary;
    //         // hdr.ipv4.srcAddr: ternary;
    //         hdr.ipv4.dstAddr: ternary;
    //         hdr.tcp_udp.srcPort: ternary;
    //         // hdr.tcp_udp.dstPort: ternary;
    //     }
    //     actions = {
    //         set_stage_params;
    //         drop;
    //         NoAction;
    //     }
    //     // size = 1024;
    //     default_action = drop();
    //     const entries = {
    //         // !important value &&& mask
    //         (1, 0x0a000303 &&& 0xffffffff, 0x0000 &&& 0x0000): set_stage_params(0x123456123456, 2);
    //     }
    // }

// ,
//   "table_entries": [
//     {
//       "table": "MyIngress.hdr_ternary",
//       "default_action": true,
//       "action_name": "MyIngress.drop",
//       "action_params": { }
//     }
//   ]
    // ,
    // {
    //   "table": "MyIngress.hdr_ternary",
    //   "match": {
    //         "hdr.sfc.chainId": 1,
    //         "hdr.ethernet.dstAddr": "0xadf" & "0xd",
    //         "hdr.ipv4.srcAddr": ternary,
    //         "hdr.ipv4.dstAddr": ternary
    //   },
    //   "action_name": "MyIngress.port_forward",
    //   "action_params": {
    //     "dstAddr": "08:00:00:00:03:00",
    //     "port": 2
    //   }
    // }
    ElementControl() elementControl;
    RouteControl() routeControl;
    apply {
        elementControl.apply(hdr, meta, standard_metadata);
        routeControl.apply(hdr, meta, standard_metadata);
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
