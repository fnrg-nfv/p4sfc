define(
	$header		"00 00 00 01 00 05",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		0,
	$flowsize	10,
);

ec :: P4SFCEncap();

// do not deny
// <action srcip dstip proto>
ipfilter :: SampleIPFilter($debug,
	allow 10.0.0.1 77.77.77.77 0x06, // tcp
	allow 10.0.0.1 77.77.77.77 0x11, // udp
)

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
    -> ipfilter;


ipfilter[0] -> [1]ec;
// ipfilter[1] -> Print(alert, ACTIVE $debug) -> [1]ec;
// ipfilter[2] -> Print(drop, ACTIVE $debug) -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
		-> pt::PrintTime(DEBUG $debug, OFFSET 4)
		-> tx :: ToDPDKDevice(0)

rx :: FromDPDKDevice(0) 
->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count) latency: $(pt.avg_latency)",
	wait 1,
	loop
	);
