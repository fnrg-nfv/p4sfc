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

ipfilter :: P4SFCIPFilter(3, $debug, 28282,
	allow 10.0.0.1 77.77.77.77 0x01,
	allow 10.0.0.1 77.77.77.77 0x02,
	allow 10.0.0.1 77.77.77.77 0x03,
	allow 10.0.0.1 77.77.77.77 0x04,
	allow 10.0.0.1 77.77.77.77 0x06, // tcp
	allow 10.0.0.1 77.77.77.77 0x05,
	allow 10.0.0.1 77.77.77.77 0x07,
	allow 10.0.0.1 77.77.77.77 0x08,
	allow 10.0.0.1 77.77.77.77 0x09,
	allow 10.0.0.1 77.77.77.77 0x0a,
	allow 10.0.0.1 77.77.77.77 0x0b,
	allow 10.0.0.1 77.77.77.77 0x0c,
	allow 10.0.0.1 77.77.77.77 0x0d,
	allow 10.0.0.1 77.77.77.77 0x0e,
	allow 10.0.0.1 77.77.77.77 0x0f,
	allow 10.0.0.1 77.77.77.77 0x10,
	allow 10.0.0.1 77.77.77.77 0x11, // udp
	allow 10.0.0.1 77.77.77.77 0x17,
	allow 10.0.0.1 77.77.77.77 0x18,
	allow 10.0.0.1 77.77.77.77 0x19,
	allow 10.0.0.1 77.77.77.77 0x1a,
	allow 10.0.0.1 77.77.77.77 0x1b,
	allow 10.0.0.1 77.77.77.77 0x1c,
	allow 10.0.0.1 77.77.77.77 0x1d,
	allow 10.0.0.1 77.77.77.77 0x1e,
	allow 10.0.0.1 77.77.77.77 0x1f,
	allow 10.0.0.1 77.77.77.77 0x21,
	allow 10.0.0.1 77.77.77.77 0x22,
	allow 10.0.0.1 77.77.77.77 0x23,
	allow 10.0.0.1 77.77.77.77 0x24,
	allow 10.0.0.1 77.77.77.77 0x26, // tcp
	allow 10.0.0.1 77.77.77.77 0x25,
	allow 10.0.0.1 77.77.77.77 0x27,
	allow 10.0.0.1 77.77.77.77 0x28,
	allow 10.0.0.1 77.77.77.77 0x29,
	allow 10.0.0.1 77.77.77.77 0x2a,
	allow 10.0.0.1 77.77.77.77 0x2b,
	allow 10.0.0.1 77.77.77.77 0x2c,
	allow 10.0.0.1 77.77.77.77 0x2d,
	allow 10.0.0.1 77.77.77.77 0x2e,
	allow 10.0.0.1 77.77.77.77 0x2f,
)

nf_from -> Strip(14) 
        -> ec
        -> CheckIPHeader
		-> IPPrint(in_ip, ACTIVE $debug)
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
