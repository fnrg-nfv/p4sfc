// src :: InfiniteSource( DATA \< 00 00 00 00 00 00 00 00 00 00 00 00 08 00 45
// 00 00 2E 00 00 40 00 40 11 1B A1 4D 4D 4D 4D 42 42 42 42 5B 25 00 50 00 1A 85
// 26 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 >, LIMIT 2, STOP
// true)

// src :: FastUDPSource(100000, 5, 60, 0:0:0:0:0:0, 77.77.77.77, 23333,
// 1:1:1:1:1:1, 66.66.66.66, 80)
src :: FromDevice(vethm1)

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
    -> ToDevice(vethm1);

ip[0] -> [0]rw;
ip[1] -> [0]rw;
ip[2] -> [1]rw;

rw[0] -> out;
