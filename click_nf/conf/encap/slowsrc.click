require(package "p4sfc");
define($header "00 00 00 03 00 20 01 70 0C DD")
define($interval 2)
define($dev eth0)
define($rate 1)
define($limit 10)
define($pktsize 1400)

mn :: SampleMonitor();

RatedSource( DATA \< 
// 00 00 00 03 00 20 01 70 0C DD
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>, LIMIT $limit, RATE 1, STOP false) 
    -> CustomEncap($header)
    -> Print(slow)
    -> mn
    -> Queue(1024)
    -> ToDevice($dev);
