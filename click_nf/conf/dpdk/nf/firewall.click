// Run: sudo bash
// Run: click --dpdk -l 8-9 -n 4 --proc-type=secondary -v -- firewall.click
define(
      $nf_id 2,
      $ipft_id 2,
      $queueSize 1024,
      $debug false,
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);
ec :: P4SFCEncap();

ipfilter :: P4SFCIPFilter($ipft_id, $debug,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/8:80 10.0.0.0/8 0x06,
	allow 10.0.0.0/30 - -,
	allow 10.0.0.0/30 - -,
	1 10.0.0.0/8 - -,
	1 - 66.66.66.66 -,
	2 - - -)

nf_from -> Strip(14) 
        -> ec
        -> CheckIPHeader
        -> ipfilter;

ipfilter[0] -> [1]ec;
ipfilter[1] -> Print(alert, ACTIVE $debug) -> [1]ec;
ipfilter[2] -> Print(drop, ACTIVE $debug) -> [1]ec;

ec[1]   -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;

Script( TYPE ACTIVE,
        print "RX: $(nf_from.count), TX: $(nf_to.n_sent)/$(nf_to.n_dropped)",
        wait 1,
  	    loop
        );
