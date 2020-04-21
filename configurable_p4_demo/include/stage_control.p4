control StageControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action set_stage(bit<8> stage_index) {
        meta.nextStage = stage_index;
    }

    action no_stage() {
        meta.nextStage = -1;
    }

    table chainId_instanceId_exact{
        actions = {
            set_stage;
            no_stage;
        }
        key = {
            hdr.sfc.chainId: exact;
            meta.nfInstanceId: exact;
        }
        size = 1024;
        default_action = no_stage();
    }
}