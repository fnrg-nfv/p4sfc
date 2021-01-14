define(
	$header		"00 00 00 01 00 05",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		0,
	$flowsize	10,
);


ips :: SampleIPS(
	$debug,
	2|00 00 00 06 00 00 00|Drives|24 00|,
	|CD 80 E8 D7 FF FF FF|/bin/sh,
	1|C0 B0|?1|DB B3 FF|1|C9 CD 80|1|C0|,
	1|C0 B0 02 CD 80 85 C0|uL|EB|L^|B0|,
	|80 00 07 00 00 00 00 00 01|?|00 01 02|,
	^|B0 02 89 06 FE C8 89|F|04 B0 06 89|F,
	|EB|V^VVV1|D2 88|V|0B 88|V|1E|,
	|EB|@^1|C0|@|89|F|04 89 C3|@|89 06|,
);

ec :: P4SFCEncap();

src :: InfiniteSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
00 00 00 01 00 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00
// 32 00 00 00 06 00 00 00 44 72 69 76 65 73 24 00
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
// 31 c0 b0 3f 31 db b3 ff 31 c9 cd 80 31 c0
// 31 c0 b0 02 cd 80 85 c0 75 4c eb 4c 5e b0
// 80 00 07 00 00 00 00 00 01 3f 00 01 02
// 5e b0 02 89 06 fe c8 89 46 04 b0 06 89 46
// eb 56 5e 56 56 56 31 d2 88 56 0b 88 56 1e
eb 40 5e 31 c0 40 89 46 04 89 c3 40 89 06
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
// 31 c0 b0 3f 31 db b3 ff 31 c9 cd 80 31 c0
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
>, LIMIT 1, STOP true, LENGTH 1400) 

src	-> Strip(14)
	-> ec
    // -> CheckIPHeader
    -> ips;

ips[0]  -> Print(Alert, ACTIVE $debug)
        -> [1]ec;
ips[1]  -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) 
		-> Print(out, ACTIVE $debug)
		-> tx :: ToDPDKDevice(0)

rx :: FromDPDKDevice(0) -> Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);

