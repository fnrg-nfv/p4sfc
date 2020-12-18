// Run: sudo bin/click --dpdk -c 0x55 -n 4 --proc-type primary -v -- conf/dpdk/dpdk-ring-primary.click

define(
	$dev	   0,
	$queueSize 102400,
	$burst     32,
	$print 	   false,
	$interval  1,
	$rate	   1
);


// for test
nicIn :: RatedSource( DATA \<
00 00 00 01 00 01
00 00 00 00 00 00 00 00 00 00 00 00 08 00
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>, RATE $rate)

// TODO: should be nic
// nicOut :: FromDPDKDevice($dev, BURST $burst);

DPDKInfo(NB_MBUF 104857, MBUF_SIZE 4096, MBUF_CACHE_SIZE 512)

// Module's I/O
nicOut :: ToDPDKDevice	($dev, IQUEUE $queueSize, BURST $burst);

// NF Classifier
// TODO: NF ids need to be confiurable
NFClf :: Classifier(2/0000, 4/0001, 4/0002, 4/0003, 4/0004, 4/0005, -)

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
nicIn -> Print("Before-NFs", ACTIVE $print) -> NFClf

// Classifier --> NFs 
NFClf[0] -> nicOut
NFClf[1] -> c::Counter -> to_nf1
NFClf[2] -> to_nf2
NFClf[3] -> to_nf3
NFClf[4] -> to_nf4
NFClf[5] -> to_nf5
NFClf[6] -> nicOut

// NFs --> Classifier
from_nf1 -> c1::Counter -> Print(" After-NF1", ACTIVE $print) -> NFClf
from_nf2 -> Print(" After-NF2", ACTIVE $print) -> NFClf
from_nf3 -> Print(" After-NF3", ACTIVE $print) -> NFClf
from_nf4 -> Print(" After-NF4", ACTIVE $print) -> NFClf
from_nf5 -> Print(" After-NF5", ACTIVE $print) -> NFClf


Script( TYPE ACTIVE,
        print "TX RATE: $(c.rate), RX RATE: $(c1.rate)",
        wait $interval,
	    loop
        );