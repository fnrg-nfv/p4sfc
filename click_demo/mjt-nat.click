// NAT composed by click elements led by mjt

src :: FromDevice(eth0);
// TODO
// out :: ToDevice(eth0);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66,
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

// TODO
out :: IPPrint(ok)
    -> Discard;

drop :: IPPrint(drop)
     -> Discard;

src -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(src)
    -> ip; 

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> drop;

rw[0] -> out;
rw[1] -> drop;
