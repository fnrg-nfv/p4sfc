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

#include <curl/curl.h>

#include "p4iprewriter.hh"

CLICK_DECLS

int P4IPRewriter::configure(Vector<String> &conf, ErrorHandler *errh) {
  // std::cout << "Specs Len: " << conf.size() << std::endl;
  for (int i = 0; i < conf.size(); ++i) {
    P4IPRewriterInput is;
    if (parse_input_spec(conf[i], is, i, errh) >= 0)
      _input_specs.push_back(is);

// #ifndef NODEBUG
//     StringAccum sa;
//     is.unparse(sa);
//     std::cout << i << ": " << conf[i].c_str() << "\tUNPARSE: " << sa.c_str()
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

void P4IPRewriter::push(int port, Packet *p_in) {

  WritablePacket *p = p_in->uniqueify();
  click_ip *iph = p->ip_header();

  if ((iph->ip_p != IP_PROTO_TCP && iph->ip_p != IP_PROTO_UDP) ||
      !IP_FIRSTFRAG(iph) || p->transport_length() < 8) {
    p->kill();
    return;
  }

  IPFlowID flowid(p);

  // 1. check in map
  P4IPRewriterEntry *entry = _map.get(flowid);

  // 2. if not in map, inputspec.output
  if (!entry) {
    P4IPRewriterInput &is = _input_specs.at_u(port);
    IPFlowID rewritten_flowid = IPFlowID::uninitialized_t();
    int result = is.rewrite_flowid(flowid, rewritten_flowid);
    if (result == rw_addmap) {
      entry = P4IPRewriter::add_flow(flowid, rewritten_flowid, port);
      if (!entry) {
        checked_output_push(port, p);
        return;
      }
    } else if (result == rw_drop)
      return;
  }

  // update the header
  entry->apply(p);

  output(entry->output()).push(p);
}

P4IPRewriterEntry *P4IPRewriter::add_flow(const IPFlowID &flowid,
                                          const IPFlowID &rewritten_flowid,
                                          int input) {
  P4IPRewriterEntry *e = new P4IPRewriterEntry();
  P4IPRewriterEntry *e_reverse = new P4IPRewriterEntry();

  e->initialize(flowid, e_reverse, _input_specs[input].foutput);
  e_reverse->initialize(rewritten_flowid, e, _input_specs[input].routput);

  _map.set(e);
  _map.set(e_reverse);

  e->p4add();
  e_reverse->p4add();

  return e;
}

int P4IPRewriterInput::rewrite_flowid(const IPFlowID &flowid,
                                      IPFlowID &rewritten_flowid) {
  int i;
  switch (kind) {
  case i_pattern: {
    i = pattern->rewrite_flowid(flowid, rewritten_flowid, owner->_map);
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
int P4IPRewriterPattern::rewrite_flowid(
    const IPFlowID &flowid, IPFlowID &rewritten_flowid,
    const HashContainer<P4IPRewriterEntry> &map) {

  rewritten_flowid = flowid;
  if (_saddr)
    rewritten_flowid.set_saddr(_saddr);
  if (_sport)
    rewritten_flowid.set_sport(_sport);
  if (_daddr)
    rewritten_flowid.set_daddr(_daddr);
  if (_dport)
    rewritten_flowid.set_dport(_dport);

  if (!_variation_top)
    return P4IPRewriter::rw_addmap;
  else {
    IPFlowID lookup = rewritten_flowid.reverse();
    uint32_t base = ntohs(_sport);

    uint32_t val;
    if (_same_first && (val = ntohs(flowid.sport()) - base) <= _variation_top) {
      lookup.set_sport(flowid.sport());
      if (!map.find(lookup)) {
        rewritten_flowid.set_sport(lookup.sport());
        return P4IPRewriter::rw_addmap;
      }
    }

    if (_sequential)
      val = (_next_variation > _variation_top ? 0 : _next_variation);
    else
      val = click_random(0, _variation_top);

    for (uint32_t count = 0; count <= _variation_top;
         ++count, val = (val == _variation_top ? 0 : val + 1)) {
      lookup.set_sport(htons(base + val));
      if (!map.find(lookup)) {
        rewritten_flowid.set_sport(lookup.dport());
        _next_variation = val + 1;
        return P4IPRewriter::rw_addmap;
      }
    }

    return P4IPRewriter::rw_drop;
  }
}

// TODO: cksm needs to be applied;
void P4IPRewriterEntry::apply(WritablePacket *p) {
  assert(p->has_network_header());
  click_ip *iph = p->ip_header();
  IPFlowID rw_flowid = _rw_entry->_flowid;

  // IP header
  iph->ip_src = rw_flowid.saddr();
  iph->ip_dst = rw_flowid.daddr();

  // click_update_in_cksum(&iph->ip_sum, 0, 0);
  // update_csum(, direction, _ip_csum_delta);

  // end if not first fragment
  if (!IP_FIRSTFRAG(iph))
    return;

  click_udp *udph = p->udp_header(); // TCP ports in the same place
  udph->uh_sport = rw_flowid.sport();
  udph->uh_dport = rw_flowid.dport();
}

void P4IPRewriterEntry::p4add() {
  IPFlowID _rw_flow = _rw_entry->_flowid;
  std::ostringstream os;
  os << "{\"instance_id\": 2,"
        "\"table_name\": \"ipRewriter.IpRewriter_exact\","
        "\"match_fields\": {"
        "\"hdr.ipv4.srcAddr\": "
     << ntohl(_flowid.saddr().addr()) << ","
     << "\"hdr.ipv4.dstAddr\": " << ntohl(_flowid.daddr().addr()) << ","
     << "\"hdr.tcp_udp.srcPort\": " << ntohs(_flowid.sport()) << ","
     << "\"hdr.tcp_udp.dstPort\": " << ntohs(_flowid.dport())
     << "}, \"action_name\":  \"ipRewriter.rewrite\","
        "\"action_params\": { "
        "\"srcAddr\":"
     << ntohl(_rw_flow.saddr().addr()) << ","
     << "\"dstAddr\":" << ntohl(_rw_flow.daddr().addr()) << ","
     << "\"srcPort\":" << ntohs(_rw_flow.sport()) << ","
     << "\"dstPort\":" << ntohs(_rw_flow.dport()) << " }}";

  std::string data = os.str(); // .str() returns temporary

  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8090/insert_entry");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    res = curl_easy_perform(curl);
    // std::cout << curl_easy_strerror(res) << std::endl;
  }
  curl_easy_cleanup(curl);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4IPRewriter)
