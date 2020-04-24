src :: FromDevice(vethm0);

outq :: Queue(20) -> Print(out)-> Discard;

rt :: RadixIPsecLookup(18.26.4.24/32 0,
		               77.77.77.77/24 77.77.77.77 1);


src -> Print(in)
    -> Paint(1)
    -> Strip(14)
    -> CheckIPHeader
    -> IPPrint(strip)
    ->rt;

rt[0] -> outq;

rt[1] 
    // -> IPsecESPEncap(FFFFFFFF) 
    // -> IPsecAuthHMACSHA1(0)
    // -> IPsecAES(1)
    // -> IPsecEncap(50)
    -> outq;
