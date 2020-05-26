/**
    Control how packets are forwarded according to sfc_id.
*/


control ForwardControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_output_port(egressSpec_t port) {
        standard_metadata.egress_spec = port;
    }

    action send_to_server() {
        standard_metadata.egress_spec = (bit<9>) 4;
    }

    table chainId_exact {
        key = {
            hdr.sfc.chainId: exact;
            hdr.sfc.chainLength: exact;
        }
        actions = {
            set_output_port;
            drop;
        }
        // size = 1024;
        default_action = drop;
        const entries = {
            (0, 1): set_output_port(3);
            (0, 4): set_output_port(3);
            (0, 0): set_output_port(2);
        }
    }

    apply {
        chainId_exact.apply();
    }
}