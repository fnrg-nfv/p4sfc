/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "include/headers.p4"
#include "include/metadata.p4"
#include "include/parser.p4"
#include "include/checksum.p4"
#include "include/deparser.p4"
#include "include/element_control.p4"
#include "include/element_complete_control.p4"
#include "include/forward_control.p4"

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
                      
    ElementControl() elementControl_0;
    ElementCompleteControl() elementCompleteControl_0;
    ElementControl() elementControl_1;
    ElementCompleteControl() elementCompleteControl_1;
    ElementControl() elementControl_2;
    ElementCompleteControl() elementCompleteControl_2;
    ElementControl() elementControl_3;
    ElementCompleteControl() elementCompleteControl_3;
    ElementControl() elementControl_4;
    ElementCompleteControl() elementCompleteControl_4;
    ForwardControl() forwardControl;
    apply {

        elementControl_0.apply(hdr, meta, standard_metadata);
        elementCompleteControl_0.apply(hdr, meta, standard_metadata);

        if(meta.hasNextElement == 1) {
            elementControl_1.apply(hdr, meta, standard_metadata);
            elementCompleteControl_1.apply(hdr, meta, standard_metadata);
        }

        if(meta.hasNextElement == 1) {
            elementControl_2.apply(hdr, meta, standard_metadata);
            elementCompleteControl_2.apply(hdr, meta, standard_metadata);
        }

        if(meta.hasNextElement == 1) {
            elementControl_3.apply(hdr, meta, standard_metadata);
            elementCompleteControl_3.apply(hdr, meta, standard_metadata);
        }

        if(meta.hasNextElement == 1) {
            elementControl_4.apply(hdr, meta, standard_metadata);
            elementCompleteControl_4.apply(hdr, meta, standard_metadata);
        }

        // if more elements need to be execute, recirculate the packet
        if(meta.hasNextElement == 1) {
            recirculate(meta);
        }
        else { //otherwise, route the packet
            if(standard_metadata.egress_spec != DROP_PORT) {
               forwardControl.apply(hdr, meta, standard_metadata);
            }
        }
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
