/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <netinet/in.h>

#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_hash.h>
#include <rte_malloc.h>
#include <rte_byteorder.h>


#include "nat_main.h"
#include "xss_nat.h"

#if defined(RTE_ARCH_X86) || defined(RTE_MACHINE_CPUFLAG_CRC32)
#define EM_HASH_CRC 1
#endif

#ifdef EM_HASH_CRC
#include <rte_hash_crc.h>
#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif


static uint32_t nat_static_public_ip = 0xbcbcbcbc;//188.188.188.188
static uint64_t nat_rule_timeout_threshold = 30; //default timeout:10s
static uint64_t statistics_show_timer_period = 10; /* default period is 10 seconds */
static uint64_t rule_timeout_check_timer_period = 30;

//per socket has a hash table ---by xss
struct rte_hash *nat_private_lookup_struct[NB_SOCKETS];
struct rte_hash *nat_public_lookup_struct[NB_SOCKETS];
static struct nat_rule *nat_rules_private[NAT_HASH_ENTRIES];
static struct nat_rule *nat_rules_public[NAT_HASH_ENTRIES];
static int port_pool[65535-1024];

struct nat_rule
{
	uint32_t private_ip;
	uint16_t private_port;
	uint32_t public_ip;
	uint16_t public_port;
	uint8_t proto;
	uint16_t assigned_port;
	uint64_t last_visited;
	//statistic data fields
	uint32_t sum_pkts_in2out;
	uint32_t sum_pkts_out2in;
};

struct nat_rule_hash_key {
	uint32_t src_ip_addr;
	uint16_t src_port;
	uint32_t dst_ip_addr;
	uint16_t dst_port;
	uint8_t proto;
};

struct nat_private_key {
	uint32_t private_ip_src;
	uint32_t public_ip_dst;
	uint16_t private_port_src;
};

struct nat_public_key {
	uint32_t public_ip_src;
	uint16_t public_port_dst;
};


static inline uint32_t
nat_hash_crc(const void *data, __rte_unused uint32_t data_len,
		uint32_t init_val)
{
	const struct nat_rule_hash_key *k;
	k = data;

#ifdef EM_HASH_CRC
	init_val = rte_hash_crc_4byte(k->src_ip_addr, init_val);
	init_val = rte_hash_crc_4byte(k->dst_ip_addr, init_val);
	init_val = rte_hash_crc_4byte((uint32_t)k->src_port, init_val);
	init_val = rte_hash_crc_4byte((uint32_t)k->dst_port, init_val);
	init_val = rte_hash_crc_4byte((uint32_t)k->proto, init_val);
#else
	init_val = rte_jhash_1word(k->src_ip_addr, init_val);
	init_val = rte_jhash_1word(k->dst_ip_addr, init_val);
	init_val = rte_jhash_1word((uint32_t)k->src_port, init_val);
	init_val = rte_jhash_1word((uint32_t)k->dst_port, init_val);
	init_val = rte_jhash_1word((uint32_t)k->proto, init_val);
#endif

	return init_val;
}

static inline uint32_t
nat_private_hash_crc(const void *data, __rte_unused uint32_t data_len,
		uint32_t init_val)
{
	const struct nat_private_key *k;
	k = data;

#ifdef EM_HASH_CRC
	init_val = rte_hash_crc_4byte(k->private_ip_src, init_val);
	init_val = rte_hash_crc_4byte(k->public_ip_dst, init_val);
	init_val = rte_hash_crc_4byte((uint32_t)k->private_port_src, init_val);
#else
	init_val = rte_jhash_1word(k->private_ip_src, init_val);
	init_val = rte_jhash_1word(k->public_ip_dst, init_val);
	init_val = rte_jhash_1word((uint32_t)k->private_port_src, init_val);
#endif
	// printf("Hash 3 tuple in calculate: %x, %x, %d\n", k->private_ip_src, k->public_ip_dst, k->private_port_src);
	// printf("Calculate result is : %d\n", init_val);

	return init_val;
}

//show NAT statistic
static void format_ip_addr(char *s, uint32_t ip){
	int addr_1 = ip >> 24;        // 提取第一部分IP地址
    ip = ip << 8;
    int addr_2 = ip >> 24;        // 提取第二部分IP地址 
    ip  = ip << 8;
    int addr_3 = ip >> 24;        // 提取第三部分IP地址 
    ip  = ip  << 8;
    int addr_4 = ip  >> 24;       // 提取第四部分IP地址 
	sprintf(s, "%d.%d.%d.%d", addr_4, addr_3, addr_2, addr_1);
}

static void print_statistic(void) {
	struct nat_rule *rule;
	char ip[32];
	printf("\nNAT Table Info================================================================================\n");
	for(int i=0; i<(65535-1024); i++){
		rule = nat_rules_private[i];
		if(rule != NULL){
			printf("NAT rule %3d: ", i);
			format_ip_addr(ip, rule->private_ip);
			printf("%15s : %5d ----> ", ip, rte_be_to_cpu_16(rule->private_port));
			format_ip_addr(ip, rule->public_ip);
			printf("%15s : %5d | assigned_port: %5d, pkt_in2out: %d, pkt_out2in: %d\n",
					ip,
					rte_be_to_cpu_16(rule->public_port),
					rte_be_to_cpu_16(rule->assigned_port),
					rule->sum_pkts_in2out,
					rule->sum_pkts_out2in);
		}
	}
	printf("==============================================================================================\n");
}

/**
10.0.0.0 - 10.255.255.255
172.16.0.0 - 172.31.255.255
192.168.0.0 - 192.168.255.255
*/
static int is_ip_private(uint32_t ip){
	ip = ip << 16;
	int addr_1 = ip >> 24;
	ip = ip << 8;
	int addr_2 = ip >> 24;
	if(addr_2==10 || (addr_2==172 && addr_1>=16 && addr_1<=31) || (addr_2==192 && addr_1==168)){
		return 1;
	}
	return 0;
}

static int nat_get_avaliable_port(void){
	for(int i=0; i<(65535-1024); i++){
		if(port_pool[i] == 0){
			port_pool[i] = 1;
			return (i+1024);
		}
	}
	return -1;
}

static inline struct nat_rule *
nat_insert_new_rule(struct nat_rule_hash_key *key,struct lcore_conf *qconf)
{
	struct nat_rule *rule = (struct nat_rule *)rte_malloc(NULL, sizeof(*rule), 0);
	rule->private_ip = key->src_ip_addr;
	rule->private_port = key->src_port;
	rule->public_ip = key->dst_ip_addr;
	rule->public_port = key->dst_port;
	rule->proto = key->proto;
	rule->assigned_port = rte_cpu_to_be_16(nat_get_avaliable_port());
	rule->last_visited = rte_rdtsc();
	rule->sum_pkts_in2out = 0;
	rule->sum_pkts_out2in = 0;
	//todo by xss
	// if(rule.assigned_port<0)
	// 	 return rule;
	
	//insert rules in both sides. i.e. in and out.
	int ret;
	ret = rte_hash_add_key((const struct rte_hash *)qconf->nat_private_lookup_struct, (const void *) key);
	nat_rules_private[ret] = rule;

	//we nned modify key for out2in
	key->src_ip_addr = key->dst_ip_addr;
	key->dst_ip_addr = nat_static_public_ip;
	key->src_port = key->dst_port;
	key->dst_port = rule->assigned_port;
	ret = rte_hash_add_key((const struct rte_hash *)qconf->nat_public_lookup_struct, (const void *) key);
	nat_rules_public[ret] = rule;

	//print insert successfully message
	char ip[32];
	format_ip_addr(ip, rule->private_ip);printf("Insert new NAT rule for : %15s : %5d ----> ",
												ip,
												rte_be_to_cpu_16(rule->private_port));
												
	format_ip_addr(ip, rule->public_ip);printf("%15s : %5d. Assigned port: %5d\n", 
												ip,
												rte_be_to_cpu_16(rule->public_port),
												rte_be_to_cpu_16(rule->assigned_port));
	return rule;
}

static inline int 
nat_private_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf){
	// printf("Receive pkt from private port...\n");
	struct nat_rule_hash_key key;
	struct ipv4_hdr *ipv4_hdr;
	bool is_tcp_packet;
	struct tcp_hdr *tcp_hdr;
	struct udp_hdr *udp_hdr;

	int ret = 0;
	ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *,
				sizeof(struct ether_hdr) + sizeof(struct sfc_hdr));

	is_tcp_packet = ipv4_hdr->next_proto_id == TCP_PROTO_ID;
	//acquire 5-tuple	
	key.src_ip_addr = ipv4_hdr->src_addr;
	key.dst_ip_addr = ipv4_hdr->dst_addr;
	key.proto = ipv4_hdr->next_proto_id;
	if(is_tcp_packet) {
		tcp_hdr = rte_pktmbuf_mtod_offset(m, struct tcp_hdr *,
						   sizeof(struct ether_hdr) + sizeof(struct sfc_hdr) + sizeof(struct ipv4_hdr));
		key.src_port = tcp_hdr->src_port;
		key.dst_port = tcp_hdr->dst_port;
	}
	else {
		udp_hdr = rte_pktmbuf_mtod_offset(m, struct udp_hdr *,
						   sizeof(struct ether_hdr) + sizeof(struct sfc_hdr) + sizeof(struct ipv4_hdr));
		key.src_port = udp_hdr->src_port;
		key.dst_port = udp_hdr->dst_port;
	}

	ret = rte_hash_lookup((const struct rte_hash *)qconf->nat_private_lookup_struct, (const void *)&key);
	struct nat_rule *rule;
	if(ret<0){
		rule = nat_insert_new_rule(&key, qconf);
	}
	else{
		rule = nat_rules_private[ret];
		rule->last_visited = rte_rdtsc();
	}
	rule->sum_pkts_in2out++;
	ipv4_hdr->src_addr = nat_static_public_ip;
	if(is_tcp_packet){
		tcp_hdr->src_port = rule->assigned_port;
	}
	else{
		udp_hdr->src_port = rule->assigned_port;
	}
	return 1;	// success!
}

static inline int 
nat_public_pkt_handler(struct rte_mbuf *m, struct lcore_conf *qconf){
	struct nat_rule_hash_key key;
	struct ipv4_hdr *ipv4_hdr;
	struct tcp_hdr *tcp_hdr;
	struct udp_hdr *udp_hdr;
	bool is_tcp_packet;
	int ret = 0;
	ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *,
				sizeof(struct ether_hdr) + sizeof(struct sfc_hdr));

	is_tcp_packet = ipv4_hdr->next_proto_id == TCP_PROTO_ID;
	key.src_ip_addr = ipv4_hdr->src_addr;
	key.dst_ip_addr = ipv4_hdr->dst_addr;
	key.proto = ipv4_hdr->next_proto_id;
	if(is_tcp_packet) {
		// tcp_hdr = (struct tcp_hdr *)((char *)ipv4_hdr + m->l3_len);
		tcp_hdr = rte_pktmbuf_mtod_offset(m, struct tcp_hdr *,
						   sizeof(struct ether_hdr) + sizeof(struct sfc_hdr) + sizeof(struct ipv4_hdr));
		key.src_port = tcp_hdr->src_port;
		key.dst_port = tcp_hdr->dst_port;
	}
	else {
		// udp_hdr = (struct udp_hdr *)((char *)ipv4_hdr + m->l3_len);
		udp_hdr = rte_pktmbuf_mtod_offset(m, struct udp_hdr *,
						   sizeof(struct ether_hdr) + sizeof(struct sfc_hdr) + sizeof(struct ipv4_hdr));
		key.src_port = udp_hdr->src_port;
		key.dst_port = udp_hdr->dst_port;
	}

	ret = rte_hash_lookup((struct rte_hash *)qconf->nat_public_lookup_struct, (const void *)&key);
	if(ret<0)
		return ret;
	// printf("Matching successfully in public direction !");
	struct nat_rule *rule = nat_rules_public[ret];
	rule->last_visited = rte_rdtsc();
	rule->sum_pkts_out2in++;
	ipv4_hdr->dst_addr = rule->private_ip;
	if(is_tcp_packet){
		tcp_hdr->dst_port = rule->private_port;
	}
	else{
		udp_hdr->dst_port = rule->private_port;
	}
	return 1;	// success

}

static void nat_rule_clear(struct nat_rule *timeout_rule, struct lcore_conf *qconf) {
	struct nat_rule_hash_key key;
	int ret;
	//in side
	key.src_ip_addr = timeout_rule->private_ip;
	key.dst_ip_addr = timeout_rule->public_ip;
	key.src_port = timeout_rule->private_port;
	key.dst_port = timeout_rule->public_port;
	key.proto = timeout_rule->proto;
	ret = rte_hash_del_key((const struct rte_hash *)qconf->nat_private_lookup_struct, (const void *) &key);
	if(ret<0) {
		printf("Error happens when delete timeout rule\n");
		return;
	}
	nat_rules_private[ret] = NULL;

	//out side
	key.src_ip_addr = timeout_rule->public_ip;
	key.dst_ip_addr = nat_static_public_ip;
	key.src_port = timeout_rule->public_port;
	key.dst_port = timeout_rule->assigned_port;
	ret = rte_hash_del_key((const struct rte_hash *)qconf->nat_public_lookup_struct, (const void *) &key);
	if(ret<0) {
		printf("Error happens when delete timeout rule\n");
		return;
	}
	nat_rules_public[ret] = NULL;

	//final step
	printf("Port %d return to nat port pool.\n", rte_be_to_cpu_16(timeout_rule->assigned_port));
	port_pool[rte_be_to_cpu_16(timeout_rule->assigned_port)-1024] = 0;
	rte_free(timeout_rule);
	
}

static void nat_rule_timeout_checker(struct lcore_conf *qconf) {
	struct nat_rule *rule;
	uint64_t cur_tsc;
	uint64_t timeout_threshold_tsc = nat_rule_timeout_threshold * rte_get_timer_hz();
	for(int i=0; i<(65535-1024); i++){
		rule = nat_rules_private[i];
		if(rule != NULL){
			cur_tsc = rte_rdtsc();
			// printf("cur %lu  last_visited %lu\n", cur_tsc, rule->last_visited);
			if(cur_tsc - rule->last_visited >= timeout_threshold_tsc) {
				nat_rule_clear(rule, qconf);
			}
		}
	}
}

// static void
// print_ethaddr(const char *name, const struct ether_addr *eth_addr)
// {
// 	char buf[ETHER_ADDR_FMT_SIZE];
// 	ether_format_addr(buf, ETHER_ADDR_FMT_SIZE, eth_addr);
// 	printf("%s%s", name, buf);
// }

/* Requirements:
 * 1. IP packets without extension;
 * 2. L4 payload should be either TCP or UDP.
 */
int
nat_check_ptype(int portid)
{
	int i, ret;
	int ptype_l3_ipv4_ext = 0;
	int ptype_l3_ipv6_ext = 0;
	int ptype_l4_tcp = 0;
	int ptype_l4_udp = 0;
	uint32_t ptype_mask = RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK;

	ret = rte_eth_dev_get_supported_ptypes(portid, ptype_mask, NULL, 0);
	if (ret <= 0)
		return 0;

	uint32_t ptypes[ret];

	ret = rte_eth_dev_get_supported_ptypes(portid, ptype_mask, ptypes, ret);
	for (i = 0; i < ret; ++i) {
		switch (ptypes[i]) {
		case RTE_PTYPE_L3_IPV4_EXT:
			ptype_l3_ipv4_ext = 1;
			break;
		case RTE_PTYPE_L3_IPV6_EXT:
			ptype_l3_ipv6_ext = 1;
			break;
		case RTE_PTYPE_L4_TCP:
			ptype_l4_tcp = 1;
			break;
		case RTE_PTYPE_L4_UDP:
			ptype_l4_udp = 1;
			break;
		}
	}

	if (ptype_l3_ipv4_ext == 0)
		printf("port %d cannot parse RTE_PTYPE_L3_IPV4_EXT\n", portid);
	if (ptype_l3_ipv6_ext == 0)
		printf("port %d cannot parse RTE_PTYPE_L3_IPV6_EXT\n", portid);
	if (!ptype_l3_ipv4_ext || !ptype_l3_ipv6_ext)
		return 0;

	if (ptype_l4_tcp == 0)
		printf("port %d cannot parse RTE_PTYPE_L4_TCP\n", portid);
	if (ptype_l4_udp == 0)
		printf("port %d cannot parse RTE_PTYPE_L4_UDP\n", portid);
	if (ptype_l4_tcp && ptype_l4_udp)
		return 1;

	return 0;
}

static inline void
nat_parse_ptype(struct rte_mbuf *m)
{
	struct ether_hdr *eth_hdr;
	uint32_t packet_type = RTE_PTYPE_UNKNOWN;
	uint16_t ether_type;
	void *l3;
	int hdr_len;
	struct ipv4_hdr *ipv4_hdr;
	struct ipv6_hdr *ipv6_hdr;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	ether_type = eth_hdr->ether_type;
	l3 = (uint8_t *)eth_hdr + sizeof(struct ether_hdr);
	if (ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {
		ipv4_hdr = (struct ipv4_hdr *)l3;
		hdr_len = (ipv4_hdr->version_ihl & IPV4_HDR_IHL_MASK) *
			  IPV4_IHL_MULTIPLIER;
		if (hdr_len == sizeof(struct ipv4_hdr)) {
			packet_type |= RTE_PTYPE_L3_IPV4;
			if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
				packet_type |= RTE_PTYPE_L4_TCP;
			else if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
				packet_type |= RTE_PTYPE_L4_UDP;
		} else
			packet_type |= RTE_PTYPE_L3_IPV4_EXT;
	} else if (ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv6)) {
		ipv6_hdr = (struct ipv6_hdr *)l3;
		if (ipv6_hdr->proto == IPPROTO_TCP)
			packet_type |= RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_TCP;
		else if (ipv6_hdr->proto == IPPROTO_UDP)
			packet_type |= RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_UDP;
		else
			packet_type |= RTE_PTYPE_L3_IPV6_EXT_UNKNOWN;
	}

	m->packet_type = packet_type;
}

uint16_t
nat_cb_parse_ptype(uint16_t port __rte_unused, uint16_t queue __rte_unused,
		  struct rte_mbuf *pkts[], uint16_t nb_pkts,
		  uint16_t max_pkts __rte_unused,
		  void *user_param __rte_unused)
{
	unsigned i;

	for (i = 0; i < nb_pkts; ++i)
		nat_parse_ptype(pkts[i]);

	return nb_pkts;
}

/* main processing loop */
//The main loop logic is very similar to the l2fwd, the difference
//between them just how to process the packet when receive the packet
//in rx queue. in this situation, the process function is 
//l3fwd_em_send_packets or l3fwd_em_no_opt_send_packets
int
nat_main_loop(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, statistics_show_timer_tsc, rule_timeout_check_timer_tsc;
	int i, nb_rx;
	uint8_t queueid;
	uint16_t portid;
	struct lcore_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) /
		US_PER_S * BURST_TX_DRAIN_US;

	prev_tsc = 0;
	statistics_show_timer_tsc = 0;
	rule_timeout_check_timer_tsc = 0;
	statistics_show_timer_period *= rte_get_timer_hz();
	rule_timeout_check_timer_period *= rte_get_timer_hz();

	lcore_id = rte_lcore_id();
	qconf = &lcore_conf[lcore_id];

	if (qconf->n_rx_queue == 0) {
		RTE_LOG(INFO, L3FWD, "lcore %u has nothing to do\n", lcore_id);
		return 0;
	}

	RTE_LOG(INFO, L3FWD, "entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_queue; i++) {

		portid = qconf->rx_queue_list[i].port_id;
		queueid = qconf->rx_queue_list[i].queue_id;
		RTE_LOG(INFO, L3FWD,
			" -- lcoreid=%u portid=%u rxqueueid=%hhu\n",
			lcore_id, portid, queueid);
	}

	while (!force_quit) {

		cur_tsc = rte_rdtsc();

		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {

			//send packets logic
			for (i = 0; i < qconf->n_tx_port; ++i) {
				portid = qconf->tx_port_id[i];
				if (qconf->tx_mbufs[portid].len == 0)
					continue;
				send_burst(qconf,
					qconf->tx_mbufs[portid].len,
					portid);
				qconf->tx_mbufs[portid].len = 0;
			}

			//print statistic logic
			if (statistics_show_timer_period > 0) {

				/* advance the timer */
				statistics_show_timer_tsc += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(statistics_show_timer_tsc >= statistics_show_timer_period)) {

					/* do this only on master core */
					if (lcore_id == rte_get_master_lcore()) {
						print_statistic();
						/* reset the timer */
						statistics_show_timer_tsc = 0;
					}
				}
			}

			//clear timeout rule logic
			if (rule_timeout_check_timer_period > 0) {

				/* advance the timer */
				rule_timeout_check_timer_tsc += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(rule_timeout_check_timer_tsc >= rule_timeout_check_timer_period)) {

					/* do this only on master core */
					if (lcore_id == rte_get_master_lcore()) {
						nat_rule_timeout_checker(qconf);
						/* reset the timer */
						rule_timeout_check_timer_tsc = 0;
					}
				}
			}
			prev_tsc = cur_tsc;
		}

		/*
		 * Read packet from RX queues
		 */
		for (i = 0; i < qconf->n_rx_queue; ++i) {
			portid = qconf->rx_queue_list[i].port_id;
			queueid = qconf->rx_queue_list[i].queue_id;
			nb_rx = rte_eth_rx_burst(portid, queueid, pkts_burst,
				MAX_PKT_BURST);
			if (nb_rx == 0)
				continue;

			nat_no_opt_send_packets(nb_rx, pkts_burst,qconf);
		}
	}

	return 0;
}

/*
 * Initialize the private/public nat hash table
 */
// this function initial hash table for every socket ---by xss
void
setup_hash(const int socketid)
{

	struct rte_hash_parameters nat_private_hash_params = {
		.name = NULL,
		.entries = NAT_HASH_ENTRIES,
		.key_len = 10,
		.hash_func = nat_hash_crc,
		.hash_func_init_val = 0,
	};

	/* create nat private hash */
	nat_private_hash_params.name = "nat_private_table";
	nat_private_hash_params.socket_id = socketid;
	nat_private_lookup_struct[socketid] =
		rte_hash_create(&nat_private_hash_params);
	if (nat_private_lookup_struct[socketid] == NULL)
		rte_exit(EXIT_FAILURE,
			"Unable to create the nat hash on socket %d\n",
			socketid);

	struct rte_hash_parameters nat_public_hash_params = {
		.name = NULL,
		.entries = NAT_HASH_ENTRIES,
		.key_len = 6,
		.hash_func = nat_hash_crc,
		.hash_func_init_val = 0,
	};

	/* create nat public hash */
	nat_public_hash_params.name = "nat_public_table";
	nat_public_hash_params.socket_id = socketid;
	nat_public_lookup_struct[socketid] =
		rte_hash_create(&nat_public_hash_params);
	if (nat_public_lookup_struct[socketid] == NULL)
		rte_exit(EXIT_FAILURE,
			"Unable to create the nat hash on socket %d\n",
			socketid);

	printf("Setup private/public nat hash table successfully in socket: %d", socketid);
}

/* Return private/public nat lookup struct. */
void *
get_nat_private_lookup_struct(const int socketid)
{
	return nat_private_lookup_struct[socketid];
}

void *
get_nat_public_lookup_struct(const int socketid)
{
	return nat_public_lookup_struct[socketid];
}
