define($interval 2)
define($dev eth0)

FastUDPFlows(1000000, -1, 1500, 0:0:0:0:0:0, 1.0.0.1, 1:1:1:1:1:1, 2.0.0.2, 100, 10)
    -> c :: Counter
    -> ToDevice($dev);

Script( TYPE ACTIVE,
        print "interval: $(interval)",
        label loop_start,
        print "TX Rate: $(c.rate); bit rate: $(c.bit_rate)",
        wait $interval,
        goto loop_start
        );
