require(package "p4sfc");
define($header "00 00 00 01 00 01")
define($interval 2)
define($dev eth0)
define($rate 1000000)
define($pktsize 61)


FastUDPFlows($rate, -1, $pktsize, 0:0:0:0:0:0, 1.0.0.1, 1:1:1:1:1:1, 2.0.0.2, 100, 10)
    -> CustomEncap($header)
    -> c :: Counter
    -> ToDevice($dev);

Script( TYPE ACTIVE,
        print "interval: $(interval)",
        label loop_start,
        print "TX Rate: $(c.rate); bit rate: $(c.bit_rate)",
        wait $interval,
        goto loop_start
        );
