#include "p4_client.h"
#include <rte_malloc.h>

general_switch_t switch_connect(char* grpc_addr, char* config_path, char* p4info_path) {
    return GS_connect(grpc_addr, config_path, p4info_path);
}

static GS_table_entry_t* build_out2in_table_entry(struct nat_rule *rule, uint32_t static_ip) {
    GS_table_entry_t *entry = (GS_table_entry_t *)rte_malloc(NULL, sizeof(*entry), 0);
    GS_match_field_lpm_t match_field_src_addr = {
        .name = "hdr.ipv4.srcAddr",
        .value = (char*)&rule->public_ip,
        .size = sizeof(rule->public_ip),
        .plen = 32,
        .next = NULL
    };
    uint32_t ip = ntohl(static_ip);
    GS_match_field_lpm_t match_field_dst_addr = {
        .name = "hdr.ipv4.dstAddr",
        .value = (char*)&ip,
        .size = sizeof(ip),
        .plen = 32,
        .next = &match_field_src_addr
    };
    GS_match_field_lpm_t match_field_src_port = {
        .name = "hdr.tcp_udp.srcPort",
        .value = (char*)&rule->public_port,
        .size = sizeof(rule->public_port),
        .plen = 16,
        .next = &match_field_dst_addr
    };
    GS_match_field_lpm_t match_field_dst_port = {
        .name = "hdr.tcp_udp.dstPort",
        .value = (char*)&rule->assigned_port,
        .size = sizeof(rule->assigned_port),
        .plen = 16,
        .next = &match_field_src_port
    };
    GS_match_field_lpm_t match_field_protocol = {
        .name = "hdr.ipv4.protocol",
        .value = (char*)&rule->proto,
        .size = sizeof(rule->proto),
        .plen = 8,
        .next = &match_field_dst_port
    };
    // build action paras
    GS_action_para_t action_para1 = {
        .name = "dstAddr",
        .value = (char*)&rule->private_ip,
        .size = sizeof(rule->private_ip),
        .next = NULL
    };
    GS_action_para_t action_para2 = {
        .name = "dstPort",
        .value = (char*)&rule->private_port,
        .size = sizeof(rule->private_port),
        .next = &action_para1
    };
    // GS_table_entry_t table_entry = {
    //     .table_name = "MyIngress.nat_exact",
    //     .action_name = "MyIngress.change_src_addr_port",
    //     .match_field_lpm = &match_field_protocol,
    //     .action_para = &action_para2
    // };
    entry->table_name = "MyIngress.nat_exact";
    entry->action_name = "MyIngress.change_dst_addr_port";
    entry->match_field_lpm = &match_field_protocol;
    entry->action_para = &action_para2;
    return entry;
}

static GS_table_entry_t* build_in2out_table_entry(struct nat_rule *rule, uint32_t static_ip) {
    printf("build_in2out_table_entry called\n");
    GS_table_entry_t *entry = (GS_table_entry_t *)rte_malloc(NULL, sizeof(*entry), 0);
    GS_match_field_lpm_t match_field_src_addr = {
        .name = "hdr.ipv4.srcAddr",
        .value = (char*)&rule->private_ip,
        .size = sizeof(rule->private_ip),
        .plen = 32,
        .next = NULL
    };
    GS_match_field_lpm_t match_field_dst_addr = {
        .name = "hdr.ipv4.dstAddr",
        .value = (char*)&rule->public_ip,
        .size = sizeof(rule->public_ip),
        .plen = 32,
        .next = &match_field_src_addr
    };
    GS_match_field_lpm_t match_field_src_port = {
        .name = "hdr.tcp_udp.srcPort",
        .value = (char*)&rule->private_port,
        .size = sizeof(rule->private_port),
        .plen = 16,
        .next = &match_field_dst_addr
    };
    GS_match_field_lpm_t match_field_dst_port = {
        .name = "hdr.tcp_udp.dstPort",
        .value = (char*)&rule->public_port,
        .size = sizeof(rule->public_port),
        .plen = 16,
        .next = &match_field_src_port
    };
    GS_match_field_lpm_t match_field_protocol = {
        .name = "hdr.ipv4.protocol",
        .value = (char*)&rule->proto,
        .size = sizeof(rule->proto),
        .plen = 8,
        .next = &match_field_dst_port
    };
    // build action paras
    uint32_t ip = ntohl(static_ip);
    GS_action_para_t action_para1 = {
        .name = "srcAddr",
        .value = (char*)&ip,
        .size = sizeof(ip),
        .next = NULL
    };
    GS_action_para_t action_para2 = {
        .name = "srcPort",
        .value = (char*)&rule->assigned_port,
        .size = sizeof(rule->assigned_port),
        .next = &action_para1
    };
    // GS_table_entry_t table_entry = {
    //     .table_name = "MyIngress.nat_exact",
    //     .action_name = "MyIngress.change_src_addr_port",
    //     .match_field_lpm = &match_field_protocol,
    //     .action_para = &action_para2
    // };
    entry->table_name = "MyIngress.nat_exact";
    entry->action_name = "MyIngress.change_src_addr_port";
    entry->match_field_lpm = &match_field_protocol;
    entry->action_para = &action_para2;
    printf("build_in2out_table_entry finished\n");
    return entry;
}

void add_nat_rule(general_switch_t s, struct nat_rule *rule, uint32_t static_ip) {
    // char *grpc_addr = "localhost:50052";
	// char *config_path = "../../build/nf.json";
	// char *p4info_path = "../../build/nf.p4.p4info.txt";
	// general_switch_t p4_switch = switch_connect(grpc_addr, config_path, p4info_path);

    GS_table_entry_t *in2out_entry = build_in2out_table_entry(rule, static_ip);
    int ret = GS_add_table_entry_lpm(s, in2out_entry);
    printf("Add in2out table entry end. Result: %d\n", ret);

    // GS_table_entry_t *out2in_entry = build_out2in_table_entry(rule, static_ip);
    // ret = GS_add_table_entry_lpm(s, in2out_entry);
    // printf("Add out2in table entry end. Result: %d\n", ret);
}

