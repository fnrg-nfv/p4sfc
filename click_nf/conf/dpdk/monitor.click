// require(package "p4sfc");
define(
    $nf_id 0,
    $queueSize 10240,
    $print false,
    $interval 1
)

// mn :: SampleMonitor();

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf1_rx, TO_PROC main_tx);
// nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf2_tx, TO_PROC main_rx, IQUEUE $queueSize);

nf_from 
    -> c :: Counter
    -> Print(mn, ACTIVE $print)
    -> Discard;

Script( TYPE ACTIVE,
        print "RX RATE: $(c.rate)",
        wait $interval,
	    loop
        );

