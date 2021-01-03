define($interval 5)
define($dev vEth0_1)

FromDevice($dev)
    -> c :: Counter
    -> Discard;

Script( TYPE ACTIVE,
        print "interval: $(interval)",
        label loop_start,
        print "RX Rate: $(c.rate); bit rate: $(c.bit_rate)",
        wait $interval,
        goto loop_start
        );

