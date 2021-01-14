define(
	$header		"00 00 00 01 00 05",
	$srcmac		0:0:0:0:0:0,
	$dstmac		0:0:0:0:0:0,
	$debug		false,
	$rate		1,
	$range		0,
	$flowsize	10,
	$length		1500,
);


ips :: SampleIPS(
	$debug,
	2|00 00 00 06 00 00 00|Drives|24 00|,
	qazwsx.hsq,
	GetInfo|0D|,
	BN|10 00 02 00|,
	WHATISIT,
	Remote|3A| ,
	Wtzup Use,
	FC ,
	host,
	USER,
	NetSphere,
	GateCrasher,
	c|3A 5C|,
	FTPON,
	FTP Port open,
	activate,
	logged in,
	|B4 B4|,
	ypi0ca,
	Ahhhh My Mouth Is Open,
	phAse zero server,
	w00w00,
	backdoor,
	r00t,
	rewt,
	wh00t!,
	lrkr0x,
	d13hh[,
	satori,
	hax0r,
	friday,
	StoogR,
	wank,
	1234,
	AAAAAAAAAA,
	PONG,
	sicken,
	ficken,
	spoofworks,
	skillz,
	login|3A|,
	l44,
	*HELLO*,
	betaalmostdone,
	gOrave,
	killme,
	gesundheit!,
	l44adsl,
	shell bound,
	alive tijgu,
	alive,
	newserver,
	stream/,
	ping,
	pong,
	>,
	>,
	>,
	|85 80 00 01 00 01 00 00 00 00|,
	|81 80|,
	|00 01 00 00 00 00 00|,
	|07|authors,
	|07|version,
	../../../,
	thisissometempspaceforthesockinaddrinyeahyeahiknowthisislamebutanywaywhocareshorizongotitworkingsoalliscool,
	ADMROCKS,
	|CD 80 E8 D7 FF FF FF|/bin/sh,
	1|C0 B0|?1|DB B3 FF|1|C9 CD 80|1|C0|,
	1|C0 B0 02 CD 80 85 C0|uL|EB|L^|B0|,
	|89 F7 29 C7 89 F3 89 F9 89 F2 AC|<|FE|,
	|EB|n^|C6 06 9A|1|C9 89|N|01 C6|F|05|,
	|90 1A C0 0F 90 02| |08 92 02| |0F D0 23 BF F8|,
	+++ath,
	|FF F4 FF FD 06|,
	/viewsource/template.html?,
	/viewsource/template.html?,
	NAMENAME,
	3|C9 B1 10|?|E9 06|Q<|FA|G3|C0|P|F7 D0|P,
	^|0E|1|C0 B0 3B 8D|~|0E 89 FA 89 F9|,
	h]^|FF D5 FF D4 FF F5 8B F5 90|f1,
	|D8|@|CD 80 E8 D9 FF FF FF|/bin/sh,
	V|0E|1|C0 B0 3B 8D|~|12 89 F9 89 F9|,
	|E8 D9 FF FF FF|/bin/sh,
	|EB|/_|EB|J^|89 FB 89|>|89 F2|,
	|EB 23|^3|C0 88|F|FA 89|F|F5 89|6,
	C|07 89|[|08 8D|K|08 89|C|0C B0 0B CD 80|1|C0 FE C0 CD 80 E8 94 FF FF FF|/bin/sh|0A|,
	XXXX%.172u%300|24|n,
	|AB CD 09 80 00 00 00 01 00 00 00 00 00 00 01 00 01|    |02|a,
	|EB 7F|]U|FE|M|98 FE|M|9B|,
	whois|3A|//,
	|EB|K[S2|E4 83 C3 0B|K|88 23 B8|Pw,
	|B4| |B4|!|8B CC 83 E9 04 8B 19|3|C9|f|B9 10|,
	from|3A 90 90 90 90 90 90 90 90 90 90 90|,
	|EB|E|EB| [|FC|3|C9 B1 82 8B F3 80|+,
	3|C9 B1 10|?|E9 06|Q<|FA|G3|C0|P|F7 D0|P,
	|01 03 00 00 00 00 00 01 00 02 02 E8|,
	|80 00 07 00 00 00 00 00 01|?|00 01 02|,
	^|B0 02 89 06 FE C8 89|F|04 B0 06 89|F,
	|EB|V^VVV1|D2 88|V|0B 88|V|1E|,
	|EB|@^1|C0|@|89|F|04 89 C3|@|89 06|,
);

ec :: P4SFCEncap();
src::P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug, 
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH $length,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 

src	-> Strip(14)
	-> ec
    -> CheckIPHeader
    -> ips;

ips[0]  -> Print(Alert, ACTIVE $debug)
        -> [1]ec;
ips[1]  -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
    	-> pt::PrintTime(DEBUG $debug, OFFSET 4)
		-> tx :: ToDPDKDevice(0)


rx::FromDPDKDevice(0)
    -> Discard

Script(
    TYPE ACTIVE,
    print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count) latency: $(pt.avg_latency)",
    wait 1,
    loop
);

