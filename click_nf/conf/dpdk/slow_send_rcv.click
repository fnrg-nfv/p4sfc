require(package "p4sfc");
define($dev 0);
define($header "00 00 00 01 00 00");
define($dstip "4D 4D 4D 4D");
define($interval 1);
define($rate 1);
define($limit -1);
define($length 1400);

// rcv
FromDPDKDevice($dev, PROMISC true)
    -> Print(in)
    -> Strip(6)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> Discard; 


// send
src :: RatedSource( DATA \< 
45 00 05 78 00 00 40 00 40 11 90 DA 0A 00 00 01 4D 4D 4D 4D 04 57 08 AE 00 1A
4E 1A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
>, LENGTH $length,  LIMIT $limit, RATE $rate, STOP false) 
    -> CustomEncap($header)
    -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
    -> Print(out)
    -> ToDPDKDevice($dev);


