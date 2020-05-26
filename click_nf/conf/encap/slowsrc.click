require(package "p4sfc");
define($header "00 00 00 01 00 01")
define($interval 2)
define($dev eth0)
define($rate 1)
define($limit 10)
define($pktsize 1494)

mn :: SampleMonitor();

RatedSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00
45 00 00 2E 00 00 40 00 40 06 96 2F 0A 00
00 01 4D 4D 4D 4D 00 00 00 00 00 00 00 00
00 00 00 00 50 00 FF FC 0B 47 00 00 00 00
00 00 00 00>, LIMIT $limit, RATE 1, STOP false) 
    -> CustomEncap($header)
    -> mn
    -> Queue(1024)
    -> ToDevice($dev);
