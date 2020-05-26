// software-NAT composed by click elements led by mjt
define($dev eth0);

require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev) -> [0]ec;
out :: EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> Print(out)
    -> [1]ec;
ec[1] -> Queue(1024)
      -> ToDevice($dev);
drop :: Print(drop)
     -> [1]ec;

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

IPRewriterPatterns(to_world_pat extern 10000-65535 - -);

// Port 0 is forwarding port
// Port 1 is dropping port
rw :: IPRewriter(// internal traffic to outside world 
                 pattern to_world_pat 0 0,
                 // outside world to internal network 
                 pass 1);

ec[0] -> Strip(14)
      -> CheckIPHeader
      -> IPPrint(src)
      -> ip; 

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> drop;

rw[0] -> out;
rw[1] -> drop;
