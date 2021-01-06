// Run: sudo click --dpdk -l 0-1 -n 4 -- simuflow.click
define(
	$dev		0,
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$flowsize	10000,
);

P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE 1, LENGTH 1500,
FLOWSIZE 15,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 
	-> Print(out, ACTIVE $debug)
	-> tx::ToDPDKDevice($dev)

rx::FromDPDKDevice($dev)
	->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
);
