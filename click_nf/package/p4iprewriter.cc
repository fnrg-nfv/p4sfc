/*
 * p4iprewriter.{cc,hh}
 * Virgil Ma
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Mazu Networks, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include <click/args.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include <clicknet/udp.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <arpa/inet.h>

#include "p4iprewriter.hh"

#define P4H_IP_SADDR "hdr.ipv4.srcAddr"
#define P4H_IP_DADDR "hdr.ipv4.dstAddr"
#define P4H_IP_SPORT "hdr.ipv4.srcPort"
#define P4H_IP_DPORT "hdr.ipv4.dstPort"
#define P4_TABLE_NAME "nat_exact"
#define P4_IPRW_ACTION_NAME "flow_rewrite"
#define P4_IPRW_PARAM_SA "srcAddr"
#define P4_IPRW_PARAM_DA "dstAddr"
#define P4_IPRW_PARAM_SP "srcPort"
#define P4_IPRW_PARAM_DP "dstPort"

CLICK_DECLS

P4IPRewriter::P4IPRewriter() {
  P4SFCState::startServer();
}

P4IPRewriter::~P4IPRewriter() {
  P4SFCState::shutdownServer();
}

int P4IPRewriter::configure(Vector<String> &conf, ErrorHandler *errh) {
  // std::cout << "Specs Len: " << conf.size() << std::endl;

  _instance_id = atoi(conf[0].c_str());

  for (int i = 1; i < conf.size(); ++i) {
    P4IPRewriterInput is;
    if (parse_input_spec(conf[i], is, i, errh) >= 0)
      _input_specs.push_back(is);

    // #ifndef NODEBUG
    //     StringAccum sa;
    //     is.unparse(sa);
    //     std::cout << i << ": " << conf[i].c_str() << "\tUNPARSE: " <<
    //     sa.c_str()
    //               << std::endl;
    // #endif
  }

  return _input_specs.size() == ninputs() ? 0 : -1;
}

int P4IPRewriter::parse_input_spec(const String &line, P4IPRewriterInput &is,
                                   int input_number, ErrorHandler *errh) {
  PrefixErrorHandler cerrh(errh, "input spec " + String(input_number) + ": ");
  String word, rest;
  if (!cp_word(line, &word, &rest))
    return cerrh.error("empty argument");
  cp_eat_space(rest);

  is.kind = P4IPRewriterInput::i_drop;
  is.owner = this;

  if (word == "drop") {
    if (rest)
      return cerrh.error("syntax error, expected %<%s%>", word.c_str());
  } else if (word == "pattern") {
    if (!P4IPRewriterPattern::parse_with_ports(rest, &is, this, &cerrh))
      return -1;
    if ((unsigned)is.foutput >= (unsigned)noutputs() ||
        (unsigned)is.routput >= (unsigned)is.owner->noutputs())
      return cerrh.error("output port out of range");
    is.pattern->use();
    is.kind = P4IPRewriterInput::i_pattern;

  } else {
    return cerrh.error("unknown specification");
  }

  return 0;
}

void flow2entry_action(const IPFlowID& flow, P4SFCState::TableEntry *e) {
  auto a = e->mutable_action();
  a->set_action(P4_IPRW_ACTION_NAME);

  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_SA);
    p->set_value(flow.saddr().s().c_str());
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_DA);
    p->set_value(flow.daddr().s().c_str());
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_SP);
    p->set_value(std::to_string(ntohs(flow.sport())));
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_DP);
    p->set_value(std::to_string(ntohs(flow.dport())));
  }

}

void flow2entry_match(const IPFlowID& flow, P4SFCState::TableEntry *e) {
  {
    auto m = e->add_match();
    m->set_field_name(P4H_IP_SADDR); // can be eliminated
    auto ex = m->mutable_exact();
    ex->set_value(flow.saddr().s().c_str());
  }
  {
    auto m = e->add_match();
    m->set_field_name(P4H_IP_DADDR); // can be eliminated
    auto ex = m->mutable_exact();
    ex->set_value(flow.daddr().s().c_str());
  }
  {
    auto m = e->add_match();
    m->set_field_name(P4H_IP_SPORT); // can be eliminated
    auto ex = m->mutable_exact();
    ex->set_value(std::to_string(ntohs(flow.sport())));
  }
  {
    auto m = e->add_match();
    m->set_field_name(P4H_IP_DPORT); // can be eliminated
    auto ex = m->mutable_exact();
    ex->set_value(std::to_string(ntohs(flow.dport())));
  }
}

void apply(WritablePacket* p, const P4SFCState::TableEntry& e) {
  assert(p->has_network_header());
  click_ip *iph = p->ip_header();

  auto a = e.action();
  {
    auto p = a.params(0);
    std::string val = p.value();
    inet_pton(AF_INET, val.c_str(), &(iph->ip_src));
  }
  {
    auto p = a.params(1);
    std::string val = p.value();
    inet_pton(AF_INET, val.c_str(), &(iph->ip_dst));
  }

  if (!IP_FIRSTFRAG(iph))
    return;
  click_udp *udph = p->udp_header(); // TCP ports in the same place

  {
    auto p = a.params(2);
    std::string val = p.value();
    udph->uh_sport = (uint16_t)std::stoi(val);
  }
  {
    auto p = a.params(3);
    std::string val = p.value();
    udph->uh_dport = (uint16_t)std::stoi(val);
  }
  // IP header
  // iph->ip_src = rw_flowid.saddr();
  // iph->ip_dst = rw_flowid.daddr();
  // click_update_in_cksum(&iph->ip_sum, 0, 0);
  // update_csum(, direction, _ip_csum_delta);
  // end if not first fragment
  // udph->uh_sport = rw_flowid.sport();
  // udph->uh_dport = rw_flowid.dport();
}

void P4IPRewriter::push(int port, Packet *p_in) {

  WritablePacket *p = p_in->uniqueify();
  click_ip *iph = p->ip_header();

  if ((iph->ip_p != IP_PROTO_TCP && iph->ip_p != IP_PROTO_UDP) ||
      !IP_FIRSTFRAG(iph) || p->transport_length() < 8) {
    p->kill();
    return;
  }

  IPFlowID flowid(p);
  P4SFCState::TableEntry lookup_entry;
  flow2entry_match(flowid, &lookup_entry);

  // 1. check in map
  P4SFCState::TableEntry* entry = _map.lookup(lookup_entry);
  // P4IPRewriterEntry *entry = _map.get(flowid);

  // 2. if not in map, inputspec.output
  if (!entry) {
    P4IPRewriterInput &is = _input_specs.at(port);
    IPFlowID rewritten_flowid = IPFlowID::uninitialized_t();
    int result = is.rewrite_flowid(flowid, rewritten_flowid);
    if (result == rw_addmap) {
      entry = add_flow(flowid, rewritten_flowid, port);
      if (!entry) {
        checked_output_push(port, p);
        return;
      }
    } else if (result == rw_drop)
      return;
  }

  // update the header
  apply(p, *entry);
  // entry->apply(p);

  // output(entry->output()).push(p);
  output(0).push(p);
}


P4SFCState::TableEntry *P4IPRewriter::add_flow(const IPFlowID &flowid, const IPFlowID &rewritten_flowid, int input) {
  P4SFCState::TableEntry *entry = P4SFCState::newTableEntry();
  P4SFCState::TableEntry *entry_r = P4SFCState::newTableEntry();
  entry->set_table_name(P4_TABLE_NAME);
  entry_r->set_table_name(P4_TABLE_NAME);
  flow2entry_match(flowid, entry);
  flow2entry_match(rewritten_flowid, entry_r);

  flow2entry_action(rewritten_flowid, entry);
  flow2entry_action(flowid, entry_r);

  _map.insert(*entry);
  _map.insert(*entry_r);

  return entry;
}

P4IPRewriterInput::P4IPRewriterInput()
    : kind(i_drop), count(0), failures(0), foutput(-1), routput(-1) {
  pattern = 0;
}

int P4IPRewriterInput::rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid) {
  int i;
  switch (kind) {
  case i_pattern: {
    i = pattern->rewrite_flowid(flowid, rewritten_flowid);
    if (i == P4IPRewriter::rw_drop)
      ++failures;
    return i;
  }
  case i_drop:
  default:
    return P4IPRewriter::rw_drop;
  }
}

void P4IPRewriterInput::unparse(StringAccum &sa) const {
  sa << "{";
  if (kind == i_drop)
    sa << "kind: drop, ";
  else if (kind == i_pattern) {
    sa << "kind: pattern, ";
    sa << "foutput: " << foutput << ", ";
    sa << "routput: " << routput << ", ";
    pattern->unparse(sa);
  }
  sa << "count: " << count << ", ";
  sa << "failures: " << failures;
  sa << "}";
}

void P4IPRewriterPattern::unparse(StringAccum &sa) const {
  sa << "[";
  sa << "(" << _saddr << " " << ntohs(_sport) << " " << _daddr << " "
     << ntohs(_dport) << "),";
  sa << " _variation_top: " << _variation_top << ",";
  sa << " _next_variation: " << _next_variation << ",";
  sa << " _sequential: " << _sequential << ",";
  sa << " _same_first: " << _same_first << ",";
  sa << " _refcount: " << _refcount << ",";
  sa << "]";
}

P4IPRewriterPattern::P4IPRewriterPattern(const IPAddress &saddr, int sport,
                                         const IPAddress &daddr, int dport,
                                         bool sequential, bool same_first,
                                         uint32_t variation)
    : _saddr(saddr), _sport(sport), _daddr(daddr), _dport(dport),
      _variation_top(variation), _next_variation(0), _sequential(sequential),
      _same_first(same_first), _refcount(0) {}

enum { PE_SYNTAX, PE_NOPATTERN, PE_SADDR, PE_SPORT, PE_DADDR, PE_DPORT };
static const char *const pe_messages[] = {
    "syntax error",    "no such pattern",         "bad source address",
    "bad source port", "bad destination address", "bad destination port"};

static inline bool pattern_error(int what, ErrorHandler *errh) {
  return errh->error(pe_messages[what]), false;
}

static bool parse_ports(const Vector<String> &words, P4IPRewriterInput *input,
                        Element *, ErrorHandler *errh) {
  if (!(words.size() == 2 && IntArg().parse(words[0], input->foutput)))
    return errh->error("bad forward port"), false;
  if (IntArg().parse(words[1], input->routput))
    return true;
  else
    return errh->error("bad reply port"), false;
}

static bool port_variation(const String &str, int32_t *port, int32_t *variation,
                           bool *sequential, bool *same_first) {
  const char *end = str.end();
  if (end > str.begin() && end[-1] == '#')
    *sequential = true, *same_first = false, --end;
  else if (end > str.begin() && end[-1] == '?')
    *same_first = false, --end;
  const char *dash = find(str.begin(), end, '-');
  int32_t port2 = 0;

  if (IntArg().parse(str.substring(str.begin(), dash), *port) &&
      IntArg().parse(str.substring(dash + 1, end), port2) && *port >= 0 &&
      port2 >= *port && port2 < 65536) {
    *variation = port2 - *port;
    return true;
  } else
    return false;
}

bool P4IPRewriterPattern::parse_with_ports(const String &str,
                                           P4IPRewriterInput *input,
                                           Element *context,
                                           ErrorHandler *errh) {
  // std::cout << str.c_str() << std::endl;

  Vector<String> words, port_words;
  cp_spacevec(str, words);

  port_words.push_back(words[words.size() - 2]);
  port_words.push_back(words[words.size() - 1]);
  words.resize(words.size() - 2);

  if (!parse_ports(port_words, input, context, errh))
    return false;

  if (words.size() != 4)
    return pattern_error(PE_SYNTAX, errh), false;

  IPAddress saddr, daddr;
  int32_t sport = 0, dport = 0, variation = 0;
  bool sequential = false, same_first = true;

  // source address
  int i = 0;
  if (!(words[i].equals("-", 1) ||
        IPAddressArg().parse(words[i], saddr, context)))
    return pattern_error(PE_SADDR, errh);

  // source port
  if (words.size() >= 3) {
    i = words.size() == 3 ? 2 : 1;
    if (!(words[i].equals("-", 1) ||
          (IntArg().parse(words[i], sport) && sport > 0 && sport < 65536) ||
          port_variation(words[i], &sport, &variation, &sequential,
                         &same_first)))
      return pattern_error(PE_SPORT, errh);
    i = words.size() == 3 ? 0 : 1;
  }

  // destination address
  ++i;
  if (!(words[i].equals("-", 1) ||
        IPAddressArg().parse(words[i], daddr, context)))
    return pattern_error(PE_DADDR, errh);

  // destination port
  if (words.size() == 4) {
    ++i;
    if (!(words[i].equals("-", 1) ||
          (IntArg().parse(words[i], dport) && dport > 0 && dport < 65536)))
      return pattern_error(PE_DPORT, errh);
  }

  input->pattern =
      new P4IPRewriterPattern(saddr, htons(sport), daddr, htons(dport),
                              sequential, same_first, variation);
  return true;
}

int P4IPRewriterPattern::rewrite_flowid(const IPFlowID& flowid, IPFlowID &rewritten_flowid) {
  rewritten_flowid = flowid;
  if (_saddr)
    rewritten_flowid.set_saddr(_saddr);
  if (_sport)
    rewritten_flowid.set_sport(_sport);
  if (_daddr)
    rewritten_flowid.set_daddr(_daddr);
  if (_dport)
    rewritten_flowid.set_dport(_dport);

  uint32_t base = ntohs(_sport);
  uint32_t val;

  if (_next_variation > _variation_top)
    return P4IPRewriter::rw_drop;
  else
    val = _next_variation;

  rewritten_flowid.set_sport(htons(base + val));
  _next_variation = val + 1;
  return P4IPRewriter::rw_addmap;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4IPRewriter)
