require(package "p4sfc");
define($dev 0);
define($header "00 00 00 01 00 00 00");
define($dstip "4D 4D 4D 4D");
define($interval 1);
define($rate 1);
define($limit -1);
define($length 1400);

// rcv
rx :: FromDPDKDevice($dev, PROMISC true)
    -> Strip(6)
    -> Strip(14)
    -> CheckIPHeader
    -> Discard; 



// send
src :: RatedSource( DATA \< 
45 00 05 78 00 00 40 00 40 11 90 DA 0A 00 00 01 4D 4D 4D 4D 04 57 08 AE 00 1A
4E 1A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
>, LENGTH $length,  LIMIT $limit, RATE $rate, STOP false) 
    -> Strip(14)
    -> CustomEncap($header)
    -> EtherEncap(0x0800, 0:0:0:0:0:0, 0:0:0:0:0:0)
    -> tx :: ToDPDKDevice($dev);


Script( TYPE ACTIVE,
        print "TX COUNT: $(tx.count); RX COUNT: $(rx.count)",
        wait $interval,
	    loop
        );
