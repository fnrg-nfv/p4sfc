/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#ifndef __XSS_NAT_H__
#define __XSS_NAT_H__

#define EGRESS_PORT 0
#define TCP_PROTO_ID 6
#define UDP_PROTO_ID 17
#define SFC_HEADER_TYPE 57100
#define IPV4_ETHERTYPE 8
#define NAT_NOT_HANDLED 128 //1000 0000 0000 0000

struct sfc_hdr {
	uint16_t is_handled;
	uint16_t ether_type;
};

static void
format_ip_addr(char *s, uint32_t ip);

static int 
is_ip_private(uint32_t ip);

static inline int 
nat_private_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf);

static inline int 
nat_public_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf);

// static void 
// print_ethaddr(const char *name, const struct ether_addr *eth_addr);


static __rte_always_inline void
nat_simple_forward(struct rte_mbuf *m, struct lcore_conf *qconf)
{
	struct sfc_hdr *sfc_hdr;
	struct ipv4_hdr *ipv4_hdr;
	int ret;
	uint16_t dst_port = EGRESS_PORT;

	sfc_hdr = rte_pktmbuf_mtod_offset(m, struct sfc_hdr *,
					sizeof(struct ether_hdr));
	if ( sfc_hdr->ether_type == IPV4_ETHERTYPE) {
		/* Handle IPv4 headers.*/
		ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *,
					sizeof(struct ether_hdr) + sizeof(struct sfc_hdr));

#ifdef DO_RFC_1812_CHECKS
		/* Check to make sure the packet is valid (RFC1812) */
		if (is_valid_ipv4_pkt(ipv4_hdr, m->pkt_len) < 0) {
			rte_pktmbuf_free(m);
			return;
		}
#endif

		if (ipv4_hdr->next_proto_id != TCP_PROTO_ID && ipv4_hdr->next_proto_id != UDP_PROTO_ID) {
			/* only handle tcp or udp packets */
			rte_pktmbuf_free(m);
			return;
		}

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
		sfc_hdr->is_handled = 0; //tell switch this pkt has been hanled.
		send_single_packet(qconf, m, dst_port);
	}
	else {
		/* Free the mbuf that contains non-IPV4 packet */
		rte_pktmbuf_free(m);
	}
}


static __rte_always_inline void
nat_packet_handler(struct rte_mbuf *m, struct lcore_conf *qconf) {
	struct ether_hdr *eth_hdr;
	struct sfc_hdr *sfc_hdr;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	// print_ethaddr("Src MAC ", &(eth_hdr->s_addr));
	// print_ethaddr("  Dst MAC ", &(eth_hdr->d_addr));
	// printf("Ether type %d\n", eth_hdr->ether_type);
	if (eth_hdr->ether_type != SFC_HEADER_TYPE) {
		printf("Bad packet format...\n");
		rte_pktmbuf_free(m);
		return;
	}

	sfc_hdr = rte_pktmbuf_mtod_offset(m, struct sfc_hdr *,
					sizeof(struct ether_hdr));
	if(sfc_hdr->is_handled != NAT_NOT_HANDLED) {
		printf("Packet has been processed by NAT...\n");
		rte_pktmbuf_free(m);
		return;
	}
	nat_simple_forward(m, qconf);
	// printf("Handle or not %d\n", sfc_hdr->is_handled);
	// printf("Ether type %d\n", sfc_hdr->ether_type);

	// struct ipv4_hdr *ipv4_hdr;
	// ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *,
	// 					   sizeof(struct ether_hdr) + sizeof(struct sfc_hdr));
	// char ip[32];
	// format_ip_addr(ip, ipv4_hdr->src_addr);
	// printf("Src IP addr %s\n", ip);
	// format_ip_addr(ip, ipv4_hdr->dst_addr);
	// printf("Dst IP addr %s\n", ip);
	// printf("Next protocol %d\n", ipv4_hdr->next_proto_id);

	// uint32_t tcp;
	// uint32_t udp;
	// tcp = m->packet_type & RTE_PTYPE_L4_TCP;
	// udp = m->packet_type & RTE_PTYPE_L4_UDP;
	// if(tcp)
	// 	printf("TCP packets!\n");
	// if(udp)
	// 	printf("UDP packets!\n");
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
		nat_packet_handler(pkts_burst[j], qconf);
	}

	/* Forward remaining prefetched packets */
	for (; j < nb_rx; j++)
		// nat_simple_forward(pkts_burst[j], qconf);
		nat_packet_handler(pkts_burst[j], qconf);
}

#endif /* __XSS_NAT_H__ */
