// Run: sudo bash
// Run: click --dpdk -l 4-5 -n 4 --proc-type=secondary -v -- pipe.click
define(
        $nf_id 1,
        $queueSize 1024,
        $debug  false,
)

ec :: P4SFCEncap();
ips :: SampleIPS(08 00 45 00, 04 05 06 07);
nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);

nf_from -> Strip(14)
        -> ec
        -> ips

ips[0]  -> Print(Alert, ACTIVE $debug)
        -> [1]ec;
ips[1]  -> [1]ec;

ec[1]   -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;

Script( TYPE ACTIVE,
        print "RX: $(nf_from.count), TX: $(nf_to.n_sent)/$(nf_to.n_dropped)",
        wait 1,
  	    loop
        );