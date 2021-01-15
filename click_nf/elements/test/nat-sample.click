define(
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		-1,
	$range		10,
	$iprw_id	1,
	$flowsize	10000,
);

ec :: P4SFCEncap();
rw :: SampleIPRewriter($debug, pattern 66.66.66.66 10000-65535 - - 0 0, drop);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src 
	-> Strip(14)
	-> ec
	-> Print(in, ACTIVE $debug)
	-> CheckIPHeader 
	-> IPPrint(in_ip, ACTIVE $debug)
	-> ip; 

ip[0] -> Discard; // [1]ec;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0]	-> CheckIPHeader	
		-> IPPrint(out_ip, ACTIVE $debug)
		-> [1]ec;

ec[1]	-> EtherEncap(0x1234, $srcmac, $dstmac)
		-> Print(out, ACTIVE $debug)
		-> pt::PrintTime(DEBUG $debug, OFFSET 4)
		-> tx :: ToDPDKDevice(0)

rx :: FromDPDKDevice(0) 
-> Discard


Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count) latency: $(pt.avg_latency)",
	wait 1,
	loop
	);
