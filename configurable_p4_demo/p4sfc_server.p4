#include <core.p4>
#include <v1model.p4>
#include "include/headers.p4"

struct metadata {
    bit<16> curNfInstanceId;
}
#include "include/parser.p4"
#include "include/checksum.p4"
#include "include/deparser.p4"


#define SWITCH_PORT 1


control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
                  
    action send_to_switch() {
        standard_metadata.egress_spec = (bit<9>) SWITCH_PORT;
    }

    action sent_to_nf_instance(bit<9> port) {
        standard_metadata.egress_spec = port;
    }

    table nf_instance_id_exact{
        actions = {
            sent_to_nf_instance;
            send_to_switch;
        }
        key = {
            meta.curNfInstanceId: exact;
        }
        // size = 1024;
        const entries = {
            (0): sent_to_nf_instance(2);
        }
        default_action = send_to_switch;
    }

    apply{
        if(hdr.sfc.chainLength == 0) {
            send_to_switch();
        }
        else {
            meta.curNfInstanceId = (bit<16>) hdr.nfs[0].nfInstanceId;
            nf_instance_id_exact.apply();
        }
    }
}

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  }
}


V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
