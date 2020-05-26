/**
    Control which element should be executed
*/

#include "elements.p4"
#include "define.p4"

control ElementControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_control_data(bit<8> elementId, bit<8> nextStage, boolean_t isNFcomplete) {
        meta.curElement = elementId;
        meta.isNFcomplete = isNFcomplete;
        meta.nextStage = nextStage;
    }

    table chainId_stageId_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
        }
        actions = {
            set_control_data;
            drop;
        }
        default_action = drop;
        // size = 1024;
        const entries = {
            (0, 1, 0): set_control_data(1, 1, 1);
            (0, 2, 0): set_control_data(2, 2, 1);
            (0, 3, 0): set_control_data(0, 255, 1);
            // (1, 1, 1): set_control_data(0, 255, 1);
            // (0, 1, 1): set_control_data(2, 2, 1);
            // (0, 2, 2): set_control_data(0, 255, 1);
        }
    }

    IpRewriter()  ipRewriter;
    Monitor()     monitor;
    Firewall()    firewall;
    Classifier()  classifier;
    IpRoute()     ipRoute;
    apply {
        chainId_stageId_exact.apply();
        if(meta.curElement == ELEMENT_NONE) {
            NoAction();
        }
        else if(meta.curElement == ELEMENT_IPREWRITER) {
            ipRewriter.apply(hdr, meta, standard_metadata);
        }
        else if (meta.curElement == ELEMENT_MONITOR) {
            monitor.apply(hdr, meta, standard_metadata);
        }
        else if(meta.curElement == ELEMENT_FIREWALL) {
            firewall.apply(hdr, meta, standard_metadata);
        }
        else if(meta.curElement == ELEMENT_CLASSIFIER) {
            classifier.apply(hdr, meta, standard_metadata);
        }
        else if(meta.curElement == ELEMENT_IPROUTE) {
            ipRoute.apply(hdr, meta, standard_metadata);
        }
    }
}