define(
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		10,
	$flowsize	10,
);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

// do not deny
ipfilter :: P4SFCIPFilter(allow src net intern && dst net intern,
                     	  1 src net intern,
                     	  1 dst host extern,
                     	  allow all)

src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src	-> Strip(14)
    -> CheckIPHeader
    -> ipfilter;

out ::	EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
		->tx :: ToDPDKDevice(0)

ipfilter[0] -> out;
ipfilter[1] -> Print(alert) -> out;

rx :: FromDPDKDevice(0) 
->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);