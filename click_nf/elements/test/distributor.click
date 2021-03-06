// Run: sudo bash
// Run: click --dpdk -l 0-3 -n 4 --proc-type=primary -v -- distributor.click
define(
	$queueSize 1024,
	$burst     32,
	$debug 	   false,
	$rate	   1,
	$header		"00 00 00 02 00 04 00 09",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$range		32,
	$flowsize	10000,
);

src :: P4SFCVariFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

tagClf:: Classifier(18/0004%fffe, -)
tag :: StoreData(14, \<F0>, \<F0>)
untagClf:: Classifier(14/f0%f0, -)
untag :: StoreData(14, \<00>, \<F0>)

// NF Classifier
NFClf :: Classifier(16/0000, 18/0002%fffe, 18/0004%fffe, 18/0006%fffe, 18/0008%fffe, 18/000a%fffe, -)

// Tx ring from Main (primary) to NF1 (secondary)
to_nf1   :: ToDPDKRing  (MEM_POOL 1, FROM_PROC main_tx, TO_PROC nf1_rx, IQUEUE $queueSize);
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

// NIC --> tag Classifier
src -> tagClf

tagClf[0] -> Print("Before-tag", ACTIVE $debug) -> tag -> Print("After-tag", ACTIVE $debug) -> NFClf
tagClf[1] -> NFClf

// Classifier --> NFs 
NFClf[0] -> pt :: PrintTime -> Discard;
NFClf[1] -> Print("Before-NF1", ACTIVE $debug) -> to_nf1
NFClf[2] -> Print("Before-NF2", ACTIVE $debug) -> to_nf2
NFClf[3] -> Print("Before-NF3", ACTIVE $debug) -> to_nf3
NFClf[4] -> Print("Before-NF4", ACTIVE $debug) -> to_nf4
NFClf[5] -> Print("Before-NF5", ACTIVE $debug) -> to_nf5
NFClf[6] -> Discard

// NFs --> Classifier
from_nf1 -> Print("After-NF1", ACTIVE $debug) -> untagClf
from_nf2 -> Print("After-NF2", ACTIVE $debug) -> untagClf
from_nf3 -> Print("After-NF3", ACTIVE $debug) -> untagClf
from_nf4 -> Print("After-NF4", ACTIVE $debug) -> untagClf
from_nf5 -> Print("After-NF5", ACTIVE $debug) -> untagClf

untagClf[0] -> untag -> Print("After-untag", ACTIVE $debug) -> Discard
untagClf[1] -> NFClf

Script( TYPE ACTIVE,
        print "TX: $(to_nf2.n_sent)/$(to_nf2.n_dropped) RX: $(from_nf2.count) latency: $(pt.avg_latency)",
        wait 1,
  	    loop
        );
