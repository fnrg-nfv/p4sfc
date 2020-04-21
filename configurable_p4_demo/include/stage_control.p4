#include "define.p4"
control StageControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action set_stage(bit<8> stage_index) {
        meta.nextStage = stage_index;
    }

    action no_stage() {
        meta.nextStage = NO_STAGE;
    }

    table chainId_instanceId_exact{
        actions = {
            set_stage;
            no_stage;
        }
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
        }
        // size = 1024;
        const entries = {
            (0, 0): set_stage(0);
        }
        default_action = no_stage();
    }

    apply{
        chainId_instanceId_exact.apply();
    }
}