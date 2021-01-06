// Run: sudo click --dpdk -l 0-1 -n 4 -- send_rcv_simu.click
define(
	$dev		0,
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$flowsize	1,
);

P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE 1, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE 1, LENGTH 1500,
FLOWSIZE 15,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 
	-> Print(out, ACTIVE $debug)
	-> latency
	-> tx::ToDPDKDevice($dev)

latency :: Latency

rx :: FromDPDKDevice($dev, PROMISC true)
	-> [1]latency

latency[1]
	-> Discard;

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
);
