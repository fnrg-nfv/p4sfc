define(
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		10,
	$flowsize	10,
);

ec :: P4SFCEncap();

// do not deny
// <action srcip:port dstip:port proto>
ipforwarder :: P4SFCIPForwarder(3, $debug,
	1 10.0.0.0/8:80,
	0 10.0.0.0/8,
	1 66.66.66.66,
	2 -)

src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SFCH \<00 00 00 01 00 05>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src	-> Strip(14)
	-> ec
    -> CheckIPHeader
    -> ipforwarder;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
		-> tx :: ToDPDKDevice(0)

ipforwarder[0] -> [1]ec;
ipforwarder[1] -> Print(alert, ACTIVE $debug) -> [1]ec;
ipforwarder[2] -> Print(drop, ACTIVE $debug) -> [1]ec;

rx :: FromDPDKDevice(0) 
->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);
