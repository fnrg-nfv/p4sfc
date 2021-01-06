define(
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
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE 1, LENGTH 1494,
FLOWSIZE $flowsize,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 
	// -> Print(out, ACTIVE $debug)
	// -> Strip(14)
	// -> CheckIPHeader
	// -> IPPrint(out, ACTIVE $debug)
    -> tx::ToDPDKDevice(0)

rx::FromDPDKDevice(0)
    ->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);