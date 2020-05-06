src :: InfiniteSource( DATA \< 
// 00 00 00 03 00 20 01 70 0C DD
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>, LIMIT 1, STOP true); 

drop :: Print(drop)
     -> Discard;

out :: Queue(1024)
    -> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
    -> Print(out)
    -> ToDevice(vethm0);

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

src -> Print(in)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(ip_in)
    -> ipclf;

ipclf[0] -> out;
ipclf[1] -> alert;
ipclf[2] -> alert;
ipclf[3] -> drop;

