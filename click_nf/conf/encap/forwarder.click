define($dev eth0);

src :: FromDevice($dev);

out :: Queue(1024)
    -> EtherEncap(0x0800, 01:02:03:04:05:06, 0a:0b:0c:0d:0e:0f)
    -> ToDevice($dev);

rt :: DirectIPLookup(18.26.4.24/32 0,
                     18.26.4.255/32 0,
                     18.26.4.0/32 0,
                     77.77.77.77/24 1);

src -> Print(in)
    -> Strip(14)
    -> CheckIPHeader
    -> GetIPAddress(16)
    -> rt;

rt[0] -> Discard;

rt[1] -> out;
