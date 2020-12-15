require(package "p4sfc");
define($dev 0);
define($header "00 00 00 01 00 01");
define($interval 2);
define($rate 1);
define($limit 1000);

// rcv
FromDPDKDevice($dev)
    -> Print(in)
    -> Strip(6)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> Discard; 



// send
RatedSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00
45 00 00 2E 00 00 40 00 40 06 96 2F 0A 00
00 01 4D 4D 4D 4D 00 00 00 00 00 00 00 00
00 00 00 00 50 00 FF FC 0B 47 00 00 00 00
00 00 00 00>, LIMIT $limit, RATE 1, STOP false) 
    -> Strip(14)
    -> CustomEncap($header)
    -> EtherEncap(0x0800, 00:00:00:00:00:00, 00:00:00:00:00:00)
    -> Print(out)
    -> ToDPDKDevice($dev);

