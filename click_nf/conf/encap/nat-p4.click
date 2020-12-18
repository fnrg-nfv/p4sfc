require(package "p4sfc");

define($dev eth0);

rw :: P4IPRewriter(770, pattern 66.66.66.66 10000-65535 - - 0 0, drop);
ec :: P4SFCEncap();

FromDevice($dev) 
	-> Print(in)
	-> Strip(14)
	-> [0]ec;
out :: Counter -> [1]ec;
ec[1] -> Print(before_eth)
      -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
      -> Print(out)
      -> Queue(1024)
      -> ToDevice($dev);

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

ec[0] -> Print(in_decap)
      -> CheckIPHeader
      -> Print(after_checkiph)
      -> IPPrint(in_ip)
      -> ip; 

ip[0] -> Print(inside1) -> out;
ip[1] -> Print(inside2) -> [0]rw;
ip[2] -> Print(inside3) -> [1]rw;
ip[3] -> Print(inside4) -> [1]rw;

rw[0] -> IPPrint(out_ip)
      -> Print(before_p4sfcencap)
      -> out;
