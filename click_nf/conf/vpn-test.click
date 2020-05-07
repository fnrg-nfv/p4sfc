src :: InfiniteSource( DATA \<00 00 00 00 00 00 00 00 00 00 00 00 08 00 45 00
00 2E 00 00 40 00 40 06 6F 22 0A 00 00 01 C0 A8 00 FF 00 00 00 00 00 00 00 00
00 00 00 00 50 00 FF FC E4 39 00 00 00 00 00 00 00 00 >, LIMIT 2, STOP true)

outq :: Queue(20) -> Print(out)-> Discard;

src -> Print(in)
    -> Paint(1)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(strip)
    -> IPsecESPEncap() 
    -> IPsecAuthHMACSHA1(0)
    -> IPsecAES(1)
    -> IPsecEncap(50)
    -> outq;
