define(
	$header		"00 00 00 01 00 05",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		0,
	$flowsize	10,
);


ips :: SampleIPS(08 00 45 00, 04 05 06 07);
ec :: P4SFCEncap();
src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src	-> Strip(14)
	-> ec
    -> CheckIPHeader
    -> ips;

ips[0]  -> Print(Alert, ACTIVE $debug)
        -> [1]ec;
ips[1]  -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
		-> tx :: ToDPDKDevice(0)

rx :: FromDPDKDevice(0) -> Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);

