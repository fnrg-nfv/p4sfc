
P4SFCSimuFlow(
SRCETH 0:0:0:0:0:0,
DSTETH 0:0:0:0:0:0,
STOP false, DEBUG true, 
LIMIT -1, RATE 1, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE 1, LENGTH 1500,
FLOWSIZE 15,
SEED 1, MAJORFLOW 0.2, MAJORDATA 0.8) 
    // -> Print(out)
    -> Strip(14)
    -> CheckIPHeader
    // -> IPPrint(ip)
    -> tx::ToDPDKDevice(0)

rx::FromDPDKDevice(0)
    ->Discard

Script( 
	TYPE ACTIVE,
	print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count)",
	wait 1,
	loop
	);