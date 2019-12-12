#ifndef __XSS_NAT_COMMON_H__
#define __XSS_NAT_COMMON_H__

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

#endif
