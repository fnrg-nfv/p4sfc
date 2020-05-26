define($dev eth0);
require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev) -> [0]ec;
ec[1] -> Queue(1024)
      -> Print(out)
      -> ToDevice($dev);

out :: EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> [1]ec;

alert :: Print(alert)
      -> out;

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ipfilter :: IPFilter(allow src net intern && dst net intern,
                     1 src net intern,
                     1 dst host extern,
                     deny all)

ec[0] -> Strip(14)
      -> CheckIPHeader
      -> ipfilter;

ipfilter[0] -> out;
ipfilter[1] -> alert;

