#include "p4_switch_helper.h"
#include <rte_malloc.h>

general_switch_t switch_connect(const char* grpc_addr, const char* config_path, const char* p4info_path, int dev_id) {
    return GS_connect(grpc_addr, config_path, p4info_path, dev_id);
}

static void insert_out2in_table_entry(general_switch_t s, struct nat_rule *rule, uint32_t static_ip) {
    GS_table_entry_t entry;
    GS_match_field_exact_t match_field_src_addr = {
        .name = "hdr.ipv4.srcAddr",
        .value = (char*)&rule->public_ip,
        .size = sizeof(rule->public_ip),
        .next = NULL
    };
    uint32_t ip = ntohl(static_ip);
    GS_match_field_exact_t match_field_dst_addr = {
        .name = "hdr.ipv4.dstAddr",
        .value = (char*)&ip,
        .size = sizeof(ip),
        .next = &match_field_src_addr
    };
    GS_match_field_exact_t match_field_src_port = {
        .name = "hdr.tcp_udp.srcPort",
        .value = (char*)&rule->public_port,
        .size = sizeof(rule->public_port),
        .next = &match_field_dst_addr
    };
    GS_match_field_exact_t match_field_dst_port = {
        .name = "hdr.tcp_udp.dstPort",
        .value = (char*)&rule->assigned_port,
        .size = sizeof(rule->assigned_port),
        .next = &match_field_src_port
    };
    GS_match_field_exact_t match_field_protocol = {
        .name = "hdr.ipv4.protocol",
        .value = (char*)&rule->proto,
        .size = sizeof(rule->proto),
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
    entry.table_name = "MyIngress.nat_exact";
    entry.action_name = "MyIngress.change_dst_addr_port";
    entry.match_field_lpm = NULL;
    entry.match_field_exact = &match_field_protocol;
    entry.action_para = &action_para2;
    int ret = GS_add_table_entry(s, &entry);
    printf("Add out2in table entry end. Result: %d\n", ret);
}

static void insert_in2out_table_entry(general_switch_t s, struct nat_rule *rule, uint32_t static_ip) {
    GS_table_entry_t entry;
    GS_match_field_exact_t match_field_src_addr = {
        .name = "hdr.ipv4.srcAddr",
        .value = (char*)&rule->private_ip,
        .size = sizeof(rule->private_ip),
        .next = NULL
    };
    GS_match_field_exact_t match_field_dst_addr = {
        .name = "hdr.ipv4.dstAddr",
        .value = (char*)&rule->public_ip,
        .size = sizeof(rule->public_ip),
        .next = &match_field_src_addr
    };
    GS_match_field_exact_t match_field_src_port = {
        .name = "hdr.tcp_udp.srcPort",
        .value = (char*)&rule->private_port,
        .size = sizeof(rule->private_port),
        .next = &match_field_dst_addr
    };
    GS_match_field_exact_t match_field_dst_port = {
        .name = "hdr.tcp_udp.dstPort",
        .value = (char*)&rule->public_port,
        .size = sizeof(rule->public_port),
        .next = &match_field_src_port
    };
    GS_match_field_exact_t match_field_protocol = {
        .name = "hdr.ipv4.protocol",
        .value = (char*)&rule->proto,
        .size = sizeof(rule->proto),
        .next = &match_field_dst_port
    };
    // build action paras
    uint32_t ip = ntohl(static_ip);
    GS_action_para_t action_para2 = {
        .name = "srcPort",
        .value = (char*)&rule->assigned_port,
        .size = sizeof(rule->assigned_port),
        .next = NULL
    };
    GS_action_para_t action_para1 = {
        .name = "srcAddr",
        .value = (char*)&ip,
        .size = sizeof(ip),
        .next = &action_para2
    };

    entry.table_name = "MyIngress.nat_exact";
    entry.action_name = "MyIngress.change_src_addr_port";
    entry.match_field_exact = &match_field_protocol;
    entry.match_field_lpm = NULL;
    entry.action_para = &action_para1;
    int ret = GS_add_table_entry(s, &entry);
    printf("Add in2out table entry end. Result: %d\n", ret);
}

void add_nat_rule(general_switch_t s, struct nat_rule *rule, uint32_t static_ip) {
    insert_in2out_table_entry(s, rule, static_ip);
    insert_out2in_table_entry(s, rule, static_ip);
}

