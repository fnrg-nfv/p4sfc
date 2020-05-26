require(package "p4sfc");
define($header "00 00 00 01 00 01")
define($interval 2)
define($dev eth0)
define($pktsize 1494)
define($rate 100000)

FastUDPSource($rate, -1, $pktsize, 0:0:0:0:0:0, 10.0.0.1, 1234, 1:1:1:1:1:1, 77.77.77.77, 1234)
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
