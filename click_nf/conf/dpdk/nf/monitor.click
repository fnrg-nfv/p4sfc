// Run: sudo bash
// Run: click --dpdk -l 4-5 -n 4 --proc-type=secondary -v -- monitor.click
define(
    $nf_id 1,
    $queueSize 1024,
    $interval 1
)

mn :: SampleMonitor();
ec :: P4SFCEncap();
nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);

nf_from -> Strip(14)
        -> ec
        -> mn
        -> [1]ec;

ec[1]   -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;
