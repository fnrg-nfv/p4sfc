require(package "p4sfc");

define($dev, eth0)

src :: FromDevice($dev);
out :: ToDevice($dev);

ips :: SampleIPS(08 00 45 00, 04 05 06 07);

src -> ips;

ips[0] -> Print(Alert) 
       -> out;
ips[1] -> out;

