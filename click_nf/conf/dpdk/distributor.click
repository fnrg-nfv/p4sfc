// Run: sudo bash
// Run: click --dpdk -l 0-3 -n 4 --proc-type=primary -v -- distributor.click
define(
	$dev	   0,
	$queueSize 1024,
	$burst     32,
	$debug 	   false,
	$interval  1,
	$rate	   1
);

nicIn	::	FromDPDKDevice	($dev, BURST $burst);
nicOut	::	ToDPDKDevice	($dev, IQUEUE $queueSize, BURST $burst);

// NF Classifier
// TODO: NF ids need to be confiurable
NFClf :: Classifier(16/0000, 18/0001, 18/0002, 18/0003, 18/0004, 18/0005, -)

// Tx ring from Main (primary) to NF1 (secondary)
to_nf1   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf1_rx, IQUEUE $queueSize, NDESC $queueSize);
to_nf2   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf2_rx, IQUEUE $queueSize);
to_nf3   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf3_rx, IQUEUE $queueSize);
to_nf4   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf4_rx, IQUEUE $queueSize);
to_nf5   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf5_rx, IQUEUE $queueSize);

// Rx ring from NF1 (secondary) back to Main (primary)
from_nf1 :: FromDPDKRing(MEM_POOL 2, FROM_PROC main_rx, TO_PROC nf1_tx, BURST $burst);
from_nf2 :: FromDPDKRing(MEM_POOL 2, FROM_PROC main_rx, TO_PROC nf2_tx, BURST $burst);
from_nf3 :: FromDPDKRing(MEM_POOL 2, FROM_PROC main_rx, TO_PROC nf3_tx, BURST $burst);
from_nf4 :: FromDPDKRing(MEM_POOL 2, FROM_PROC main_rx, TO_PROC nf4_tx, BURST $burst);
from_nf5 :: FromDPDKRing(MEM_POOL 2, FROM_PROC main_rx, TO_PROC nf5_tx, BURST $burst);

// NIC --> Classifier
nicIn -> Print("Before-NFs", ACTIVE $debug) -> NFClf

// Classifier --> NFs 
NFClf[0] -> nicOut
NFClf[1] -> to_nf1
NFClf[2] -> to_nf2
NFClf[3] -> to_nf3
NFClf[4] -> to_nf4
NFClf[5] -> to_nf5
NFClf[6] -> nicOut

// NFs --> Classifier
from_nf1 -> Print("After-NF1", ACTIVE $debug) -> NFClf
from_nf2 -> Print("After-NF2", ACTIVE $debug) -> NFClf
from_nf3 -> Print("After-NF3", ACTIVE $debug) -> NFClf
from_nf4 -> Print("After-NF4", ACTIVE $debug) -> NFClf
from_nf5 -> Print("After-NF5", ACTIVE $debug) -> NFClf


Script( TYPE ACTIVE,
        print "TX: $(nicIn.count), RX: $(nicOut.count)",
        wait $interval,
	    loop
        );