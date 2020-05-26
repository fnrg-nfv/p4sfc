define($dev eth0)

src :: FromDevice($dev)

drop :: Discard;

out :: Queue(1024)
    -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> ToDevice($dev);

alert :: Print(alert)
      -> out;

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ipclf :: IPClassifier(src net intern and dst net intern,
                      src net intern,
                      dst host extern,
                      -);

ipfilter :: IPFilter(deny src 10.0.0.1/8,
                     deny all)


src -> Strip(14)
    -> CheckIPHeader
    -> ipclf;

ipclf[0] -> out;
ipclf[1] -> alert;
ipclf[2] -> alert;
ipclf[3] -> drop;

