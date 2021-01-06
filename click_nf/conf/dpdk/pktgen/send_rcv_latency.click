// Run: sudo click --dpdk -l 0-1 -n 4 -- send_rcv_simu.click
define(
	$dev		0,
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$srcip		10.0.0.1,
	$dstip		77.77.77.77,
	$range		1,
	$length		1500,
	$debug		false,
	$flowsize	1,
	$rate		1,
);

latency :: Latency

P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP $srcip, DSTIP $dstip, RANGE $range, LENGTH $length,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 
	-> latency
	-> tx::ToDPDKDevice($dev)

rx :: FromDPDKDevice($dev, PROMISC true)
	-> [1]latency

latency[1] -> Discard;

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
);
