control ElementCompleteControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    apply {
        meta.stageId = meta.stageId + 1;
        if (meta.isNFcomplete == 1) {
            hdr.nfs.pop_front(1); // remove completed nf
        }
    }
}