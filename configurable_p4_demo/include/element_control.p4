#include "elements.p4"
#include "define.p4"

control ElementControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_control_data(bit<8> elementId, boolean_t hasNextElement, boolean_t isNFcomplete) {
        meta.curElement = elementId;
        meta.isNFcomplete = isNFcomplete;
        meta.hasNextElement = hasNextElement;
    }

    table chainId_stageId_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.stageId: exact;
        }
        actions = {
            set_control_data;
            drop;
        }
        // size = 1024;
        default_action = drop;

        const entries = {
            (0, 0): set_control_data(0, 1, 0);
            (0, 1): set_control_data(0, 1, 0);
            (0, 2): set_control_data(0, 1, 0);
            (0, 3): set_control_data(0, 1, 0);
            (0, 4): set_control_data(0, 1, 0);
            (0, 5): set_control_data(0, 0, 0);
        }
    }

    IpRewriter() ipRewriter;
    apply {
        chainId_stageId_exact.apply();
        if(meta.curElement == ELEMENT_IPREWRITER) {
            ipRewriter.apply(hdr, meta, standard_metadata);
        }
    }
}