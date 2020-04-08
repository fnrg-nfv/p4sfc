control RouteControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_output_port(egressSpec_t port) {
        standard_metadata.egress_spec = port;
    }

    table chainId_exact {
        key = {
            hdr.sfc.chainId: exact;
        }
        actions = {
            set_output_port;
            drop;
        }
        // size = 1024;
        default_action = drop;
        const entries = {
            (1): set_output_port(2);
        }
    }

    apply {
        chainId_exact.apply();
    }
}