// Run: sudo click --dpdk -l 0-3 -n 4 -- send_rcv.click
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
// 00 00 00 00 00 00 00 00 00 00 00 00 12 34 
// $header
// 45 00 00 2E 00 00 40 00 40 06 96 2F
// $srcip $dstip $srcport $dstport 
// 00 00 00 00 00 00 00 00 50 00 FF FC 0B 47 
// 00 00 00 00 00 00 00 00
// >, LENGTH $length,  LIMIT $limit, RATE $rate, STOP false) 
// 	-> Print(out, ACTIVE $debug)
// 	-> tx :: ToDPDKDevice($dev);

//Create a UDP flow of $N packets
FastUDPFlows(RATE $rate, LIMIT $limit, LENGTH $length, SRCETH 0:0:0:0:0:0,
DSTETH 0:0:0:0:0:0, SRCIP 10.0.0.1, DSTIP 77.77.77.77, FLOWS 10, FLOWSIZE 100)
-> MarkMACHeader
//EnsureDPDKBuffer will copy the packet inside a DPDK buffer, so there is no more copies (not even to the NIC) afterwards when we replay the packet many time
-> EnsureDPDKBuffer
//MutliReplayUqueue pulls all packets from its input, and replay them from memory $S amount of time
-> replay :: MultiReplayUnqueue(STOP -1, ACTIVE false, QUICK_CLONE 1)
-> tx :: ToDPDKDevice($dev);
// -> td :: ToDPDKDevice(0, BLOCKING $blocking, VERBOSE $verbose)

// rcv
rx :: FromDPDKDevice($dev, PROMISC true)
   	-> Print(in, ACTIVE $debug)
	-> Strip(20)
	-> CheckIPHeader
	-> IPPrint(in_ip, ACTIVE $debug)
	-> Discard;

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count); RX: $(rx.count)",
	wait $interval,
	loop
	);
