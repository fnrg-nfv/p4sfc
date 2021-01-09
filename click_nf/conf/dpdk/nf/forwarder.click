// Run: sudo bash
// Run: click --dpdk -l 9-10 -n 4 --proc-type=secondary -v -- forwarder.click
define(
      $nf_id 2,
      $queueSize 1024,
      $debug false
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);
ec :: P4SFCEncap();

// rt :: DirectIPLookup(18.26.4.24/32 0,
//                      18.26.4.255/32 0,
//                      18.26.4.0/32 0,
//                      66.66.66.66/32 1,
//                      77.77.77.77/24 1);

ipforwarder :: P4SFCIPForwarder($nf_id, $debug,
	1 10.0.0.0/8:80,
	0 10.0.0.0/8,
	1 66.66.66.66,
	2 -)

nf_from -> Strip(14)
        -> ec
        -> CheckIPHeader
        -> ipforwarder;

ipforwarder[0] -> Print(drop, ACTIVE $debug) -> [1]ec;
ipforwarder[1] -> [1]ec;
ipforwarder[2] -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;

Script(
      TYPE ACTIVE,
      print "RX: $(nf_from.count), TX: $(nf_to.n_sent)/$(nf_to.n_dropped)",
      wait 1,
      loop
);
