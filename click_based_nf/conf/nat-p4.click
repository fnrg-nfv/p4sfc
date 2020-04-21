require(package "p4sfc");

rw :: P4IPRewriter(pattern 66.66.66.66 10000-65535 - - 0 0, drop);
ec :: P4SFCEncap();

// src :: InfiniteSource(
// DATA \< 00 00 00 03 00 20 01 70 0C DD
// 00 00 00 00 00 00 00 00 00 00 00 00 08 00 
// 45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
// 00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
// 00 00 00 00 00 00 00 00 00 00 00 00 00 00
// 00 00 00 00>, LIMIT 2, STOP true)
src :: FromDevice(ens32:0);
// src :: FromDevice(ens33);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

src -> Print(ip)
    -> [0]ec;
out :: IPPrint(out_ip)
    -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> [1]ec;
ec[1] -> Print(out)
      -> Queue(1024)
      -> ToDevice(ens32:0);
      // -> ToDevice(ens33);
      // -> Discard;
ec[0] -> Strip(14)
      -> CheckIPHeader
      -> IPPrint(in_ip)
      -> ip; 

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> out;
