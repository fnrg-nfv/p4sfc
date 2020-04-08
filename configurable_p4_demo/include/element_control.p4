#include "elements.p4"
#include "define.p4"

control ElementControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_control_data(bit<8> elementId, bit<1> hasNextElement) {
        meta.curElement = elementId;
        meta.hasNextElement = hasNextElement;
    }
    table chainId_exact{
        key = {
            hdr.sfc.chainId: exact;
        }
        actions = {
            set_control_data;
            drop;
        }
        // size = 1024;
        default_action = drop;

        const entries = {
            (1): set_control_data(1, 1);
        }
    }

    IpRewriter() ipRewriter;
    apply {
        chainId_exact.apply();
        if(meta.curElement == ELEMENT_IPREWRITER) {
            ipRewriter.apply(hdr, meta, standard_metadata);
        }
    }
}