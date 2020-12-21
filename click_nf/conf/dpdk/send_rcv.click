// Run: sudo click --dpdk -l 0-3 -n 4 --proc-type=primary -v -- distributor.click
define(
	$dev 0,
	$header "00 00 00 01 00 00",
	$srcip "0A 00 00 01",
	$dstip "4D 4D 4D 4D",
	$srcport "00 00",
	$dstport "00 00",
	$interval 1,
	$rate 1,
	$limit -1,
	$length 64,
	$debug false
);

// send
// src :: RatedSource( DATA \< 
// ethernet(14)
// 00 00 00 00 00 00 00 00 00 00 00 00 08 00
// $header
// 45 00 00 2E 00 00 40 00 40 06 96 2F 
// $srcip
// $dstip 
// $srcport
// $dstport 
// 00 00 00 00 00 00 00 00 50 00 FF FC 0B 47
// 00 00 00 00 00 00 00 00>,
// LENGTH $length,  LIMIT $limit, RATE $rate, STOP false) 

src::FastUDPSource($rate, $limit, $length, 0:0:0:0:0:0, 1.0.0.1, 1234, 1:1:1:1:1:1, 2.0.0.2, 1234)
	-> Print(out, ACTIVE $debug)
	-> tx :: ToDPDKDevice($dev);

// rcv
rx :: FromDPDKDevice($dev, PROMISC true)
   	-> Print(in, ACTIVE $debug)
	-> Strip(18)
	-> CheckIPHeader
	-> IPPrint(in_ip, ACTIVE $debug)
	-> Discard;

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count); RX: $(rx.count)",
	wait $interval,
	loop
	);
