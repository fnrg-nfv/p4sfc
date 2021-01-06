// Run: sudo bash
// Run: click --dpdk -l 8-9 -n 4 --proc-type=secondary -v -- firewall.click
define(
      $nf_id 3,
      $queueSize 1024,
      $debug false
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);
ec :: P4SFCEncap();

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ipfilter :: IPFilter(allow src net intern && dst net intern,
                     1 src net intern,
                     1 dst host extern,
                     deny all)

nf_from     -> ec;
            -> Strip(14)
            -> CheckIPHeader
            -> ipfilter;

ipfilter[0] -> [1]ec;
ipfilter[1] -> Print(alert) -> [1]ec;

ec[1]   -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;
