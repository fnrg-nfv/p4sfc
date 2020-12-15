require(package "p4sfc");

rw :: P4IPRewriter(10, pattern 66.66.66.66 10000-65535 - - 0 0, drop);


src :: InfiniteSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00 45 00 00 2E 00 00 40 00 40 11 96 24
0A 00 00 01 4D 4D 4D 4D 6E 79 22 B8 00 1A C9 ED 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
>, LIMIT 3, STOP false); 

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

// TODO: what about src extern and dst intern;
ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

src -> Print(in)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> ip; 

out :: IPPrint(out_ip)
    -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> Print(out)
    -> Discard;

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> out;
