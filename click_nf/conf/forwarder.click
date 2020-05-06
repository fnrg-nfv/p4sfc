src :: InfiniteSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>, LIMIT 1, STOP true); 

out :: Queue(1024)
    -> EtherEncap(0x0800, 01:02:03:04:05:06, 0a:0b:0c:0d:0e:0f)
    -> Print(out)
    -> ToDevice(vethm0);

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
