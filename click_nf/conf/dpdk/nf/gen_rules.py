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


def gen_firewall_rules(n=100):
    """Generate n firewall rules from the most-specific rule
    to the most-general rule.
    As we need to test the throughput, all rules are allow.
    """
    if (n - 1) % 3 != 0:
        raise RuntimeError("rule size should be: 3x+1...")

    rules = []
    rule_pattern = "allow %s/%d 77.77.77.77/8 0x06"

    network_ip_len = 32
    for i in range(3):
        for i in range((n - 1) / 3):
            rule = rule_pattern % (gen_ip(range=100), network_ip_len)
            rules.append(rule)
        network_ip_len /= 2

    rules.append("allow - - -")

    return rules


if __name__ == '__main__':
    rules = gen_firewall_rules(10)
    with open("./rules", "w") as f:
        for rule in rules:
            f.write(rule)
            f.write("\n")
