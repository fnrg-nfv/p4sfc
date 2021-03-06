require(package "p4sfc");
define($dev, eth0)

rw :: P4IPRewriter(0, pattern 66.66.66.66 10000-65535 - - 0 0, drop);

src :: FromDevice($dev);

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

src -> Print(ip)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> ip; 

out :: IPPrint(out_ip)
    -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> Print(out)
    -> Queue(1024)
    -> ToDevice($dev);

ip[0] -> out;
ip[1] -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> out;
