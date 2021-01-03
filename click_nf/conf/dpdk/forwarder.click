// Run: sudo bash
// Run: click --dpdk -l 9-10 -n 4 --proc-type=secondary -v -- forwarder.click
define(
      $nf_id 4,
      $queueSize 1024,
      $debug false
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);
ec :: P4SFCEncap();

rt :: DirectIPLookup(18.26.4.24/32 0,
                     18.26.4.255/32 0,
                     18.26.4.0/32 0,
                     66.66.66.66/32 1,
                     77.77.77.77/24 1);

nf_form     -> ec;
            -> Print(in)
            -> Strip(14)
            -> CheckIPHeader
            -> GetIPAddress(16)
            -> rt;

rt[0] -> Print(drop) -> [1]ec;
rt[1] -> [1]ec;

ec[1] -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0) -> nf_to;
