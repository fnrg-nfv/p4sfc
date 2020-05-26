define($dev eth0);
require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev) -> [0]ec;
out :: Print(out)
    -> [1]ec;
ec[1] -> Queue(1024)
      -> ToDevice($dev);

out1 :: EtherEncap(0x0800, 01:02:03:04:05:06, 0a:0b:0c:0d:0e:0f)
     -> out;

rt :: DirectIPLookup(18.26.4.24/32 0,
                     18.26.4.255/32 0,
                     18.26.4.0/32 0,
                     66.66.66.66/32 1,
                     77.77.77.77/24 1);

ec[0] -> Print(in)
      -> Strip(14)
      -> CheckIPHeader
      -> GetIPAddress(16)
      -> rt;

rt[0] -> Print(drop)
      -> Discard;

rt[1] -> out1;
