
src :: P4SFCSimuFlow( 
SRCETH 0:0:0:0:0:0,
DSTETH 0:0:0:0:0:0,
LIMIT -1, RATE 100000000, STOP false, DEBUG true, FLOWSIZE 4, LENGTH 1500) 
    // -> Print(out)
    -> Strip(14)
    -> CheckIPHeader
    // -> IPPrint(ip)
    -> tx::ToDPDKDevice(0)
    // -> Discard

rx::FromDPDKDevice(0)
    ->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count); RX: $(rx.count)",
	wait 1,
	loop
	);