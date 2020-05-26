/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>

static volatile bool force_quit;

#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct rte_ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t l2fwd_enabled_port_mask = 0;

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct rte_mempool * l2fwd_pktmbuf_pool = NULL;

struct p4sfc_chain_header {
	uint16_t chain_id;
	uint16_t chain_length;
};

struct p4sfc_nf_header {
	uint16_t nf_id;
};

static uint64_t timer_period = 5; /* default period is 10 seconds for send packets */
static bool latency_test = true;

static void
fill_p4sfc_header(struct rte_mbuf *m, struct p4sfc_chain_header *hdr) {
	uint16_t chain_length = 1;
	hdr->chain_id = rte_cpu_to_be_16(0);
	hdr->chain_length = rte_cpu_to_be_16(chain_length);
	uint16_t i;
	struct p4sfc_nf_header *nf_hdr;
	uint16_t nf_id;
	for (i = 0; i < chain_length; i++) {
		nf_hdr = (struct p4sfc_nf_header *) rte_pktmbuf_append(m, sizeof(struct p4sfc_nf_header));
		if (i == chain_length - 1) {
			nf_id = (i << 1) + 1; // is last
		}
		else {
			nf_id = i << 1;
		}
		nf_hdr->nf_id = rte_cpu_to_be_16(nf_id);
	}
}

static void
fill_ethernet_header(struct rte_ether_hdr *hdr) {
	struct rte_ether_addr s_addr = {{0x14,0x02,0xEC,0x89,0x8D,0x24}};
	struct rte_ether_addr d_addr = {{0x00,0x50,0x56,0x97,0x5A,0xBF}};
	hdr->s_addr =s_addr;
	hdr->d_addr =d_addr;
	hdr->ether_type = rte_cpu_to_be_16(0x0800);
}

static void
fill_ipv4_header(struct rte_ipv4_hdr *hdr) {
	hdr->version_ihl = (4 << 4) + 5; // ipv4, length 5 (*4)
	hdr->type_of_service = 0; // No Diffserv
	hdr->total_length = rte_cpu_to_be_16(40); // tcp 20
	hdr->packet_id = rte_cpu_to_be_16(5462); // set random
	hdr->fragment_offset = rte_cpu_to_be_16(0);
	hdr->time_to_live = 64;
	hdr->next_proto_id = 6; // tcp
	hdr->hdr_checksum = rte_cpu_to_be_16(25295);
	// hdr->src_addr = rte_cpu_to_be_32(0xC0A80001); // 192.168.0.1
	hdr->src_addr = rte_cpu_to_be_32(0x0AA80001);
	hdr->dst_addr = rte_cpu_to_be_32(0x01010101); // 1.1.1.1
}

static void
fill_tcp_header(struct rte_tcp_hdr *hdr) {
	hdr->src_port = rte_cpu_to_be_16(0x162E);
	hdr->dst_port = rte_cpu_to_be_16(0x04d2);
	hdr->sent_seq = rte_cpu_to_be_32(0);
	hdr->recv_ack = rte_cpu_to_be_32(0);
	hdr->data_off = 0;
	hdr->tcp_flags = 0;
	hdr->rx_win = rte_cpu_to_be_16(16);
	hdr->cksum = rte_cpu_to_be_16(0);
	hdr->tcp_urp = rte_cpu_to_be_16(0);
}

static void
p4sfc_send_custom_pkt_burst(void)
{
	unsigned dst_port, i;
	struct rte_eth_dev_tx_buffer *buffer;
	struct rte_mbuf *m;
	dst_port = 0;
	buffer = tx_buffer[dst_port];
	struct p4sfc_chain_header *chain_h;
	struct rte_ether_hdr *ether_h;
	struct rte_ipv4_hdr *ipv4_h;
	struct rte_tcp_hdr *tcp_h;
	unsigned burst_size;
	if (latency_test) {
		burst_size = 1;
	}
	else {
		burst_size = MAX_PKT_BURST - 1;
	}
	for (i = 0; i < burst_size; i++){
		m = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool);

		chain_h = (struct p4sfc_chain_header *) rte_pktmbuf_append(m, sizeof(struct p4sfc_chain_header));
		fill_p4sfc_header(m, chain_h);

		ether_h = (struct rte_ether_hdr *) rte_pktmbuf_append(m, sizeof(struct rte_ether_hdr));
		fill_ethernet_header(ether_h);

		ipv4_h = (struct rte_ipv4_hdr *) rte_pktmbuf_append(m, sizeof(struct rte_ipv4_hdr));
		fill_ipv4_header(ipv4_h);

		tcp_h = (struct rte_tcp_hdr *) rte_pktmbuf_append(m, sizeof(struct rte_tcp_hdr));
		fill_tcp_header(tcp_h);

		rte_eth_tx_buffer(dst_port, 0, buffer, m);
	}
}


/* main processing loop */
static void
l2fwd_main_loop_latency(void)
{
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc;
	struct rte_eth_dev_tx_buffer *buffer;
	int sent;

	prev_tsc = 0;
	lcore_id = rte_lcore_id();

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	while (!force_quit) {

		cur_tsc = rte_rdtsc();

		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > timer_period)) {
			// send custom packet
			p4sfc_send_custom_pkt_burst();
			buffer=tx_buffer[0];
			struct timeval te;
			gettimeofday(&te, NULL); // get current time
			long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
			printf("Send a packet\n  Time: %lld ms\n", milliseconds);
			sent = rte_eth_tx_buffer_flush(0, 0, buffer);
			if (sent > 0) {
				// printf("Send %d packets...\n", sent);
			}
			prev_tsc = cur_tsc;
		}
	}
}

static void
l2fwd_main_loop_throughput(void)
{
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
			BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;
	int sent;

	prev_tsc = 0;
	lcore_id = rte_lcore_id();

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	while (!force_quit) {

		cur_tsc = rte_rdtsc();

		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {
			// send custom packet
			p4sfc_send_custom_pkt_burst();
			buffer=tx_buffer[0];
			sent = rte_eth_tx_buffer_flush(0, 0, buffer);
			if (sent > 0) {
				// printf("Send %d packets...\n", sent);
			}
			prev_tsc = cur_tsc;
		}
	}
}


static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	if (latency_test) {
		l2fwd_main_loop_latency();
	}
	else {
		l2fwd_main_loop_throughput();
	}
	return 0;
}

/* display usage */
static void
l2fwd_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n",
	       prgname);
}

static int
l2fwd_parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

static const char short_options[] =
	"p:"  /* portmask */
	;


/* Parse the argument given in the command line of the application */
static int
l2fwd_parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	char *prgname = argv[0];

	argvopt = argv;

	while ((opt = getopt(argc, argvopt, short_options)) != EOF) {

		switch (opt) {
		/* portmask */
		case 'p':
			l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
			if (l2fwd_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* long options */
		case 0:
			break;

		default:
			l2fwd_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 1; /* reset getopt lib */
	return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint16_t portid;
	uint8_t count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;
	int ret;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		RTE_ETH_FOREACH_DEV(portid) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			ret = rte_eth_link_get_nowait(portid, &link);
			if (ret < 0) {
				all_ports_up = 0;
				if (print_flag == 1)
					printf("Port %u link get failed: %s\n",
						portid, rte_strerror(-ret));
				continue;
			}
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf(
					"Port%d Link Up. Speed %u Mbps - %s\n",
						portid, link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n", portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

int
main(int argc, char **argv)
{
	int ret;
	uint16_t nb_ports;
	uint16_t nb_ports_available = 0;
	uint16_t portid;
	unsigned lcore_id;
	unsigned int nb_lcores = 0;
	unsigned int nb_mbufs;

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");


	/* convert to number of cycles */
	timer_period *= rte_get_timer_hz();

	nb_ports = rte_eth_dev_count_avail();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* check port mask to possible port mask */
	if (l2fwd_enabled_port_mask & ~((1 << nb_ports) - 1))
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
			(1 << nb_ports) - 1);

	nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + MAX_PKT_BURST +
		nb_lcores * MEMPOOL_CACHE_SIZE), 8192U);

	/* create the mbuf pool */
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	/* Initialise each port */
	RTE_ETH_FOREACH_DEV(portid) {
		struct rte_eth_rxconf rxq_conf;
		struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", portid);
			continue;
		}
		nb_ports_available++;

		/* init port */
		printf("Initializing port %u... ", portid);
		fflush(stdout);

		ret = rte_eth_dev_info_get(portid, &dev_info);
		if (ret != 0)
			rte_exit(EXIT_FAILURE,
				"Error during getting device (port %u) info: %s\n",
				portid, strerror(-ret));

		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, portid);

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
						       &nb_txd);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot adjust number of descriptors: err=%d, port=%u\n",
				 ret, portid);

		ret = rte_eth_macaddr_get(portid,
					  &l2fwd_ports_eth_addr[portid]);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot get MAC address: err=%d, port=%u\n",
				 ret, portid);

		/* init one RX queue */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     &rxq_conf,
					     l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, portid);

		/* init one TX queue on each port */
		fflush(stdout);
		txq_conf = dev_info.default_txconf;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				&txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, portid);

		/* Initialize TX buffers */
		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_dev_set_ptypes(portid, RTE_PTYPE_UNKNOWN, NULL,
					     0);
		if (ret < 0)
			printf("Port %u, Failed to disable Ptype parsing\n",
					portid);
		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, portid);

		printf("done: \n");

		ret = rte_eth_promiscuous_enable(portid);
		if (ret != 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_promiscuous_enable:err=%s, port=%u\n",
				 rte_strerror(-ret), portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				portid,
				l2fwd_ports_eth_addr[portid].addr_bytes[0],
				l2fwd_ports_eth_addr[portid].addr_bytes[1],
				l2fwd_ports_eth_addr[portid].addr_bytes[2],
				l2fwd_ports_eth_addr[portid].addr_bytes[3],
				l2fwd_ports_eth_addr[portid].addr_bytes[4],
				l2fwd_ports_eth_addr[portid].addr_bytes[5]);

	}

	if (!nb_ports_available) {
		rte_exit(EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	check_all_ports_link_status(l2fwd_enabled_port_mask);

	ret = 0;
	/* launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL, CALL_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	RTE_ETH_FOREACH_DEV(portid) {
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");

	return ret;
}
