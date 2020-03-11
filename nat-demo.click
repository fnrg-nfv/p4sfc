
// ARCHITECTURE
// in -> headerClassifier -> ipClassifier -> rw     -> output
//                       \-> drop        \->drop   \-> drop

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66	66.66.66.66/24,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);


IPRewriterPatterns(to_world_pat extern 10000-65535 - -);

rw :: IPRewriter(// internal traffic to outside world
		 pattern to_world_pat 0 0,
         // outside world to internal network
         // if no mapping, pass to dropping port
		 pass 1);

src :: InfiniteSource(
DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>,
LIMIT 5, STOP true);

src -> Print(src)
    -> Strip(14)
    -> CheckIPHeader
    -> ip; 

out :: Print(ok)
    -> Discard;

drop :: Print(drop)
    -> Discard;

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> drop;

rw[0] -> out;
rw[1] -> drop;
