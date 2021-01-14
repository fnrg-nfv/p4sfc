from random import choice, randint, shuffle
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
    rule_pattern = "allow %s %s 0x11,"

    ip_src_base = ipv4_2_int("10.0.0.1")
    ip_dst_base = ipv4_2_int("77.77.77.77")
    for i in range(100):
        ip_src = ip_src_base + i
        for j in range(100):
            ip_dst = ip_dst_base + j
            rule = rule_pattern % (int_2_ipv4(ip_src), int_2_ipv4(ip_dst))
            rules.append(rule)
    return rules


def gen_forwarder_rules():
    """Generate forwarder rules.
    As we need to test the throughput, all rules are forward.
    """
    rules = []
    rule_pattern = "128 %s,"

    ip_dst_base = ipv4_2_int("77.77.77.77")
    for i in range(10000):
        ip_dst = ip_dst_base + i
        rule = rule_pattern % (int_2_ipv4(ip_dst))
        rules.append(rule)
    shuffle(rules)
    return rules


if __name__ == '__main__':
    rules = gen_forwarder_rules()
    with open("./rules", "w") as f:
        for rule in rules:
            f.write(rule)
            f.write("\n")
