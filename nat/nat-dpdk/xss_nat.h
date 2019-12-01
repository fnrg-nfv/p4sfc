/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#ifndef __XSS_NAT_H__
#define __XSS_NAT_H__

#define EGRESS_PORT 0

struct sfc_hdr {
	uint16_t is_handled;
	uint16_t ether_type;
};

static int 
is_ip_private(uint32_t ip);

static inline int 
nat_private_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf);

static inline int 
nat_public_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf);

static void 
print_ethaddr(const char *name, const struct ether_addr *eth_addr);


static __rte_always_inline void
nat_simple_forward(struct rte_mbuf *m, struct lcore_conf *qconf)
{
	// struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	int ret;
	uint16_t dst_port = EGRESS_PORT;
	uint32_t tcp;
	uint32_t udp;
	uint32_t l3_ptypes;

	// eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
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

		if(is_ip_private(ipv4_hdr->src_addr)){
			ret = nat_private_pkt_handler(m, qconf);
		}
		else{
			ret = nat_public_pkt_handler(m, qconf);
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

		send_single_packet(qconf, m, dst_port);
	}
	else {
		/* Free the mbuf that contains non-IPV4 packet */
		rte_pktmbuf_free(m);
	}
}


static __rte_always_inline void
nat_user_defined_header_handler(struct rte_mbuf *m) {
	struct ether_hdr *eth_hdr;
	struct sfc_hdr *sfc_hdr;
	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	print_ethaddr("Src MAC ", &(eth_hdr->s_addr));
	print_ethaddr("Dst MAC ", &(eth_hdr->d_addr));
	printf("Ether type %d\n", eth_hdr->ether_type);

	sfc_hdr = rte_pktmbuf_mtod_offset(m, struct sfc_hdr *,
					sizeof(struct ether_hdr));
	printf("Handle or not %d\n", sfc_hdr->is_handled);
	printf("Ether type %d\n", sfc_hdr->ether_type);
}

/*
 * Buffer non-optimized handling of packets, invoked
 * from main_loop.
 */
static inline void
nat_no_opt_send_packets(int nb_rx, struct rte_mbuf **pkts_burst, struct lcore_conf *qconf)
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
		// nat_simple_forward(pkts_burst[j], qconf);
		nat_user_defined_header_handler(pkts_burst[j]);
	}

	/* Forward remaining prefetched packets */
	for (; j < nb_rx; j++)
		// nat_simple_forward(pkts_burst[j], qconf);
		nat_user_defined_header_handler(pkts_burst[j]);
}

#endif /* __XSS_NAT_H__ */
