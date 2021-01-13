define(
	$header		"00 00 00 01 00 00",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		-1,
	$range		10,
	$iprw_id	1,
	$flowsize	1000,
	$port		28282,
);

ec :: P4SFCEncap();
rw :: P4IPRewriter($iprw_id, $debug, $port, pattern - - 192.168.0.1-192.168.0.255 - 0 0);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	77.77.77.77  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1400,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src -> Strip(14)
	-> ec
	-> Print(in, ACTIVE $debug)
	-> CheckIPHeader 
	-> IPPrint(in_ip, ACTIVE $debug)
	-> rw; 

rw[0]	-> CheckIPHeader
		-> IPPrint(out_ip, ACTIVE $debug)
		-> [1]ec;

ec[1]	-> EtherEncap(0x1234, $srcmac, $dstmac)
		-> Print(out, ACTIVE $debug)
		-> tx :: ToDPDKDevice(0)

rx :: FromDPDKDevice(0) -> Discard


Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);
