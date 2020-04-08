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
                      
    ElementControl() elementControl_1;
    ElementControl() elementControl_2;
    RouteControl() routeControl;
    apply {
        elementControl_1.apply(hdr, meta, standard_metadata);
        
        if(meta.hasNextElement == 1) {
            // elementControl_2.apply(hdr, meta, standard_metadata);
            elementControl_2.apply(hdr, meta, standard_metadata);
        }

        // more elements here

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
