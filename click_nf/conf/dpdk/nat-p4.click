// Run: sudo bash
// Run: click --dpdk -l 8-11 -n 4 --proc-type=secondary -v -- nat-p4.click
define(
      $id 2,
      $queueSize 1024
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(id)_tx, TO_PROC main_rx, IQUEUE $queueSize);

rw :: P4IPRewriter($id, pattern 66.66.66.66 10000-65535 - - 0 0, drop);
ec :: P4SFCEncap();

nf_from -> Print(in)
	-> Strip(14)
	-> [0]ec;
ec[1] -> Print(before_eth)
      -> EtherEncap(0x1234, extern:eth, extern_next_hop:eth)
      -> Print(out)
      -> nf_to;

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

ec[0] -> Print(in_decap)
      -> CheckIPHeader
      -> Print(after_checkiph)
      -> IPPrint(in_ip)
      -> ip; 

ip[0] -> [1]ec;
ip[1] -> Print(ip2rw) -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> IPPrint(out_ip)
      -> Print(before_p4sfcencap)
      -> [1]ec;
