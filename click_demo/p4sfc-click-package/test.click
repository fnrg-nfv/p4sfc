require(package "p4sfc");

rw :: P4IPRewriter(pattern 66.66.66.66 10000-65535 - - 0 0, drop);

src :: InfiniteSource(
DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>,
LIMIT 5, STOP true);

out :: IPPrint(ok)
    -> Discard;

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

src -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(src)
    -> ip; 

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> out;
