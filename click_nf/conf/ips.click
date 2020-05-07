require(package "p4sfc");

src :: InfiniteSource( DATA \< 00 00 00 00 00 00 00 00 00 00 00 00 08 00 45
00 00 2E 00 00 40 00 40 11 96 24 0A 00 00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD
41 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00>, LIMIT 2, STOP
true)

ips :: SampleIPS(00 01 02 03, 04 05 06 07);

src -> ips;

ips[0] -> Print(Alert) 
       -> Discard;
ips[1] -> Print(Out)
       -> Discard;
