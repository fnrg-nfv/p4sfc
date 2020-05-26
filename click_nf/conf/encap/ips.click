define($dev eth0);
require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev) -> [0]ec;
out :: Print(out)
    -> [1]ec;
ec[1] -> Queue(1024)
      -> ToDevice($dev);

ips :: SampleIPS(08 00 45 00, 04 05 06 07);

ec[0] -> ips;

ips[0] -> Print(Alert) 
       -> out;
ips[1] -> out;

