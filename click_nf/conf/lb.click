define($dev eth0);

src :: FromDevice($dev);

rr_mapper :: RoundRobinIPMapper (
  - - 10.0.0.1 80 0 0,
  - - 10.0.0.2 80 0 0,
  - - 10.0.0.3 80 0 0 );


rw :: IPRewriter(rr_mapper, drop);

AddressInfo(
  lb      66.66.66.66,
  intern 	10.0.0.0/8,
  server1 10.0.0.1,
  server2 10.0.0.2,
  server3 10.0.0.3,
);

ip :: IPClassifier(src net intern,
                   dst host lb,
                   -);

src -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(in_ip)
    -> ip;

out :: IPPrint(out_ip)
    -> EtherEncap(0x0800, 01:02:03:04:05:06, 0a:0b:0c:0d:0e:0f)
    -> Queue(1024)
    -> ToDevice($dev);

ip[0] -> [0]rw;
ip[1] -> [0]rw;
ip[2] -> [1]rw;

rw[0] -> out;

