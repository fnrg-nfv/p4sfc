//
// Virtual Private Networking using IPSec
//

// zwolle's side

// 18.26.4.0 -> zeus -> IPSec Tunnel -> redlab -> 18.26.4.200
// 18.26.4.0 -> 10.0.0.1 -> IPSec Tunnel -> 10.0.0.2 -> 18.26.4.200
//
// IPSec Security Policy Database
//
// 0. Packets destined for tsb
// 1. Tunnel packets from redlab to us
// 2. ARP packets

src :: InfiniteSource(
DATA \<00 00 00 00 00 00 00 00 00
00 00 00 08 00 45 00 00 2E 00 00 40
00 40 11 0D C4 12 1A 04 00 12 1A 04
C8 5B 25 22 B8 00 1A 54 E1 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00>,
LIMIT 5, STOP true);


spd :: Classifier(12/0800 30/121a04c8,
              12/0800 30/121a0461 26/121a040a,
              12/0806 20/0002,
              -);

src -> [0]spd;
outq :: Queue(20) -> Print(out)-> Discard;

//
// IPSec Incoming SAD (Security Association Database)
// 
//
// 0. ESP Packets to zeus with SPI 0x00000001
//

isad :: Classifier(9/32 16/121a0461 20/00000001,
               9/32,
	       -);
//
// IPSec Outgoing SAD (Security Association Database)
// 
//
// 0. Any packets to 18.26.4.200
//

osad :: Classifier(16/121a04c8,
	      -);

// Common code to send IP packets, including ARP.
// IP dst annotation must already be set.
arpq :: ARPQuerier(18.26.4.97, 00:00:c0:ca:68:ef);
spd[2] -> [1]arpq;
arpq[0] -> outq;


isad[0] -> Strip(20)
        -> IPsecDES(ENCRYPT 1)
        -> IPsecESPUnencap
        -> Print(rtun)
        -> GetIPAddress(16)
        // -> StaticIPLookup(18.26.4.22 18.26.4.22)
	-> [0]arpq;

isad[1] -> Discard;

isad[2] -> Print(Not_in_SAD)
        -> Discard;

osad[0] -> Print(tun)
        -> IPsecESPEncap()
        -> IPsecAuthHMACSHA1(0)
        -> IPsecAES(1)
        -> IPsecEncap(50)
        -> [0]arpq;

// osad[0] -> Print(tun)
//         // -> Annotation
//         // -> IPsecESPEncap() //(0x00000001, 8)
//         -> Print(tun)
//         -> IPsecDES(ENCRYPT 0)
//         -> IPEncap(50, 18.26.4.97, 18.26.4.10)
//         -> GetIPAddress(16)
//         // -> StaticIPLookup(18.26.4.10 18.26.4.10)
//         -> [0]arpq;

osad[1] -> Print("How'd we get here?")
        -> Discard;


spd[0] -> Strip(14)
       -> [0]osad;
spd[1] -> Strip(14)
       -> [0]isad;
spd[3] -> outq;

























