// Run: sudo click --dpdk -l 0-3 -n 4 -- send_rcv.click
define(
	$dev 0,
	$header "00 00 00 01 00 00",
	$srcmac 0:0:0:0:0:0,
	$dstmac 0:0:0:0:0:0,
	$srcip 10.0.0.1,
	$dstip 77.77.77.77,
	$interval 1,
	$rate 1,
	$limit -1,
	$length 1400,
	$debug false,
	$flows 10,
	$flowsize 100,
);


//Create a UDP flow
FastUDPFlows(RATE $rate, LIMIT $limit, LENGTH $length, SRCETH $srcmac,
			DSTETH $dstmac, SRCIP $srcip, DSTIP $dstip, FLOWS $flows, FLOWSIZE $flowsize)
	-> Strip(14)
	-> Print(out, ACTIVE $debug)
	-> CustomEncap($header)
	-> Print(out, ACTIVE $debug)
	-> EtherEncap(0x1234, $srcmac, $dstmac)
	-> Unqueue()
	-> tx :: ToDPDKDevice($dev);

// rcv
// rx :: FromDPDKDevice($dev)
// 	-> Discard;

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)",
	wait $interval,
	loop
	);
