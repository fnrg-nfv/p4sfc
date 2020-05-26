src :: InfiniteSource( DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 45 00
00 2E 00 00 40 00 40 06 6F 22 0A 00 00 01 C0 A8 00 FF 00 00 00 00 00 00 00 00
00 00 00 00 50 00 FF FC E4 39 00 00 00 00 00 00 00 00 >, LIMIT 1, STOP true)

outq :: Queue(20) -> Print(out)-> Discard;

rt :: RadixIPsecLookup(192.168.0.255/32 0);
		      //   18.26.8.0/24 18.26.4.1 1 234 ABCDEFFF001DEFD2354550FE40CD708E 112233EE556677888877665544332211 300 64);

src -> Print(in)
    -> Paint(1)
    -> Strip(14)
    -> CheckIPHeader(INTERFACES 192.168.0.255/24)
    -> IPPrint(strip)
    -> rt
    -> IPsecESPEncap(SPI 12345678) 
    // -> IPsecAuthHMACSHA1(0)
    // -> IPsecAES(1)
    // -> IPsecEncap(50)
    -> outq;
