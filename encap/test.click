require(package "p4sfc");

ec :: P4SFCEncap();

// src :: InfiniteSource(
// DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 
// 45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
// 00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
// 00 00 00 00 00 00 00 00 00 00 00 00 00 00
// 00 00 00 00>,
// LIMIT 5, STOP true);
src :: FromDevice(eth0);

src -> Print(src)
    -> [0]ec;

ec[0] -> Print(ec0)
      -> [1]ec;

ec[1] -> Print(ec1)
      -> Discard;
