/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#ifndef __XSS_NAT_H__
#define __XSS_NAT_H__

static inline int 
nat_modify_pkt_private(struct rte_mbuf *m, struct lcore_conf *qconf);

static inline int 
nat_modify_pkt_public(struct rte_mbuf *m, struct lcore_conf *qconf);

static __rte_always_inline void
nat_simple_forward(struct rte_mbuf *m, bool public_port, uint16_t dest_port,
		struct lcore_conf *qconf)
{
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	int ret;
	uint16_t dst_port = dest_port;
	uint32_t tcp;
	uint32_t udp;
	uint32_t l3_ptypes;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	tcp = m->packet_type & RTE_PTYPE_L4_TCP;
	udp = m->packet_type & RTE_PTYPE_L4_UDP;
	l3_ptypes = m->packet_type & RTE_PTYPE_L3_MASK;

	if ( (tcp || udp) && (l3_ptypes == RTE_PTYPE_L3_IPV4)) {
		/* Handle IPv4 headers.*/
		ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *,
						   sizeof(struct ether_hdr));

#ifdef DO_RFC_1812_CHECKS
		/* Check to make sure the packet is valid (RFC1812) */
		if (is_valid_ipv4_pkt(ipv4_hdr, m->pkt_len) < 0) {
			rte_pktmbuf_free(m);
			return;
		}
#endif
		if(public_port){
			ret = nat_modify_pkt_public(m, qconf);
		}
		else{
			ret = nat_modify_pkt_private(m, qconf);
		}
		if(ret<0){
			rte_pktmbuf_free(m);
			return;
		}
		//send pkt logic

#ifdef DO_RFC_1812_CHECKS
		/* Update time to live and header checksum */
		--(ipv4_hdr->time_to_live);
		++(ipv4_hdr->hdr_checksum);
#endif
		/* dst addr */
		*(uint64_t *)&eth_hdr->d_addr = dest_eth_addr[dst_port];

		/* src addr */
		ether_addr_copy(&ports_eth_addr[dst_port], &eth_hdr->s_addr);

		send_single_packet(qconf, m, dst_port);
	}
	else {
		/* Free the mbuf that contains non-IPV4 packet */
		rte_pktmbuf_free(m);
	}
}

/*
 * Buffer non-optimized handling of packets, invoked
 * from main_loop.
 */
static inline void
nat_no_opt_send_packets(int nb_rx, struct rte_mbuf **pkts_burst,
			bool public_port, uint16_t dest_port, struct lcore_conf *qconf)
{
	int32_t j;

	/* Prefetch first packets */
	for (j = 0; j < PREFETCH_OFFSET && j < nb_rx; j++)
		rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));

	/*
	 * Prefetch and forward already prefetched
	 * packets.
	 */
	for (j = 0; j < (nb_rx - PREFETCH_OFFSET); j++) {
		rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[
				j + PREFETCH_OFFSET], void *));
		nat_simple_forward(pkts_burst[j], public_port, dest_port, qconf);
	}

	/* Forward remaining prefetched packets */
	for (; j < nb_rx; j++)
		nat_simple_forward(pkts_burst[j], public_port, dest_port, qconf);
}

#endif /* __XSS_NAT_H__ */
