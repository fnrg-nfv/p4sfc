define($dev eth0);

FromDevice($dev)
    -> Print(in)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> Discard; 

