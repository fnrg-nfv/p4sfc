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
	300,
|00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00|
);

ec :: P4SFCEncap();

src :: InfiniteSource( DATA \< 
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
00 00 00 01 00 00 
// 73 65 6e 64 7c 47 7c 7c 43 6f 6e 74 7c 61 63 74 73 7c
00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 
55 73 65 72 2d 41 67 65 6e 74 3a 20 4e 65 74 53 75 70 70 6f 72 74 20 4d 61 6e 61 67 65 72 2f
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00
// 31 c0 b0 3f 31 db b3 ff 31 c9 cd 80 31 c0
// 31 c0 b0 02 cd 80 85 c0 75 4c eb 4c 5e b0
// 80 00 07 00 00 00 00 00 01 3f 00 01 02
// 5e b0 02 89 06 fe c8 89 46 04 b0 06 89 46
// eb 56 5e 56 56 56 31 d2 88 56 0b 88 56 1e
// eb 40 5e 31 c0 40 89 46 04 89 c3 40 89 06
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
// 31 c0 b0 3f 31 db b3 ff 31 c9 cd 80 31 c0
// cd 80 e8 d7 ff ff ff 2f 62 69 6e 2f 73 68
>, LIMIT 1, STOP true, LENGTH 64) 

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

