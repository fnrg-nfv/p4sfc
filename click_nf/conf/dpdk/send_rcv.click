define($dev 0);
define($header "00 00 00 01 00 00 00");
define($dstip "4D 4D 4D 4D");
define($interval 1);
define($rate 1);
define($limit -1);
define($length 64);
define($print false);
define($counter ACTIVE);

// rcv
rx :: FromDPDKDevice($dev, PROMISC true)
//   	-> Print(in, ACTIVE $print)
//	-> Strip(14)
//	-> Strip(4)
//	-> CheckIPHeader
//	-> IPPrint(in_ip, ACTIVE $print)
	-> Discard; 



// send
src :: RatedSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00
00 00 00 00 
45 00 00 2E 00 00 40 00 40 06 96 2F 0A 00
00 01 
$dstip 
00 00 00 00 00 00 00 00
00 00 00 00 50 00 FF FC 0B 47 00 00 00 00
00 00 00 00>, LENGTH $length,  LIMIT $limit, RATE $rate, STOP false) 
//	-> Print(out, ACTIVE $print)
	-> tx :: ToDPDKDevice($dev);

Script( 
	TYPE $counter,
      	print "TX: $(tx.count); RX: $(rx.count)",
      	wait $interval,
	loop
      );
