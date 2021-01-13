from random import choice, randint
import socket
import struct
import argparse


def ipv4_2_int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


def int_2_ipv4(addr):
    return socket.inet_ntoa(struct.pack("!I", addr))


def gen_ip(base="10.0.0.1", range=1):
    return int_2_ipv4(ipv4_2_int(base) + randint(0, range))


def gen_firewall_rules():
    """Generate firewall rules.
    As we need to test the throughput, all rules are allow.
    """
    rules = []
    rule_pattern = "(0, 3, 0, %s, %s): allow();"

    ip_src_base = ipv4_2_int("10.0.0.1")
    ip_dst_base = ipv4_2_int("77.77.77.77")
    for i in range(32):
        ip_src = ip_src_base + i
        for j in range(32):
            ip_dst = ip_dst_base + j
            rule = rule_pattern % (ip_src, ip_dst)
            rules.append(rule)
    return rules

def gen_forwarder_rules():
    """Generate forwarder rules.
    As we need to test the throughput, all rules are allow.
    """
    rules = []
    rule_pattern = "(0, 4, 0, %s): set_output_port(128);"

    ip_dst_base = ipv4_2_int("77.77.77.77")
    for i in range(32):
        for j in range(32):
            ip_dst = ip_dst_base + j
            rule = rule_pattern % (ip_dst)
            rules.append(rule)
    return rules

if __name__ == '__main__':
    rules = gen_forwarder_rules()
    with open("./rules", "w") as f:
        for rule in rules:
            f.write(rule)
            f.write("\n")
