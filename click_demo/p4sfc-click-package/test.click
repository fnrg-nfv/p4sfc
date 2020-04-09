require(package "p4sfc");

test :: SamplePackageElement;

rw :: P4IPRewriter(pattern 66.66.66.66 10000-65535 - - 0 0);

src :: InfiniteSource(
DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>,
LIMIT 5, STOP true);

out :: IPPrint(ok)
    -> Discard;

src -> test 
    -> rw
    -> out;
