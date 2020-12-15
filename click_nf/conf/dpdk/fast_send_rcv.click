// TODO: not implemented currently
require(package "p4sfc");
define($dev 0);
define($header "00 00 00 01 00 01");
define($interval 2);
define($rate 10);
define($pktsize 60);

// rcv
FromDPDKDevice($dev)
    -> c :: Counter
    -> Print(in)
    -> Strip(14)
//    -> Strip(6)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> Discard; 


// send
FastUDPSource($rate, -1, $pktsize, 0:0:0:0:0:0, 10.0.0.1, 1234, 1:1:1:1:1:1, 77.77.77.77, 1234)
    -> Counter
//    -> Strip(14)
//    -> CustomEncap($header)
//    -> EtherEncap(0x0800, 00:00:00:00:00:00, 00:00:00:00:00:00)
    -> Print(out)
    -> ToDPDKDevice($dev);


