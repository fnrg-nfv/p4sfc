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

#include "p4iprewriter.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include <iostream>

CLICK_DECLS

P4IPRewriter::P4IPRewriter() {}

P4IPRewriter::~P4IPRewriter() {}

int P4IPRewriter::initialize(ErrorHandler *errh) {
  errh->message("Successfully linked with P4IPRewriter!");
  return 0;
}

int P4IPRewriter::configure(Vector<String> &conf, ErrorHandler *errh) {
  std::cout << "Specs Len: " << conf.size() << std::endl;
  for (int i = 0; i < conf.size(); ++i) {
    P4IPRewriterInput is;
    if (parse_input_spec(conf[i], is, i, errh) >= 0)
      _input_specs.push_back(is);

#ifndef NODEBUG
    StringAccum sa;
    is.unparse(sa);
    std::cout << i << ": " << conf[i].c_str() << "\tUNPARSE: " << sa.c_str()
              << std::endl;
#endif
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

void P4IPRewriter::push(int port, Packet *p) {
  // 1. check in map;

  // 2. if not in map, inputspec.output

  output(port).push(p);
}

void P4IPRewriterInput::unparse(StringAccum &sa) const {
  if (kind == i_drop)
    sa << "kind: drop";
  else if (kind == i_pattern) {
    sa << "kind: pattern";
    sa << " foutput: " << foutput;
    sa << " routput: " << routput;
    pattern->unparse(sa);
  }
  sa << " count: " << count;
  sa << " failures: " << failures;
}

void P4IPRewriterPattern::unparse(StringAccum &sa) const {
  sa << "[";
  sa << "(" << _saddr << " " << ntohs(_sport) << " " << _daddr << " "
     << ntohs(_dport) << "),";
  sa << " _variation_top: " << _variation_top << ",";
  sa << " _next_variation: " << _next_variation << ",";
  sa << " _is_napt: " << _is_napt << ",";
  sa << " _sequential: " << _sequential << ",";
  sa << " _same_first: " << _same_first << ",";
  sa << " _refcount: " << _refcount << ",";
  sa << "]";
}

P4IPRewriterPattern::P4IPRewriterPattern(const IPAddress &saddr, int sport,
                                         const IPAddress &daddr, int dport,
                                         bool is_napt, bool sequential,
                                         bool same_first, uint32_t variation)
    : _saddr(saddr), _sport(sport), _daddr(daddr), _dport(dport),
      _variation_top(variation), _next_variation(0), _is_napt(is_napt),
      _sequential(sequential), _same_first(same_first), _refcount(0) {}

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

static bool ip_address_variation(const String &str, IPAddress *addr,
                                 int32_t *variation, bool *sequential,
                                 bool *same_first, Element *context) {
  const char *end = str.end();
  if (end > str.begin() && end[-1] == '#')
    *sequential = true, *same_first = false, --end;
  else if (end > str.begin() && end[-1] == '?')
    *same_first = false, --end;
  const char *dash = find(str.begin(), end, '-');
  IPAddress addr2;

  if (dash != end &&
      IPAddressArg().parse(str.substring(str.begin(), dash), *addr, context) &&
      IPAddressArg().parse(str.substring(dash + 1, end), addr2, context) &&
      ntohl(addr2.addr()) >= ntohl(addr->addr())) {
    *variation = ntohl(addr2.addr()) - ntohl(addr->addr());
    return true;
  } else if (dash == end &&
             IPPrefixArg(true).parse(str.substring(str.begin(), end), *addr,
                                     addr2, context) &&
             *addr && addr2 && addr2.mask_to_prefix_len() >= 0) {
    if (addr2.addr() == 0xFFFFFFFFU)
      *variation = 0;
    else if (addr2.addr() == htonl(0xFFFFFFFEU)) {
      *addr &= addr2;
      *variation = 1;
    } else {
      // don't count PREFIX.0 and PREFIX.255
      *addr = (*addr & addr2) | IPAddress(htonl(1));
      *variation = ~ntohl(addr2.addr()) - 2;
    }
    return true;
  } else
    return false;
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
  std::cout << str.c_str() << std::endl;

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
        IPAddressArg().parse(words[i], saddr, context) ||
        ip_address_variation(words[i], &saddr, &variation, &sequential,
                             &same_first, context)))
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

  input->pattern = new P4IPRewriterPattern(saddr, htons(sport), daddr,
                                           htons(dport), words.size() >= 3,
                                           sequential, same_first, variation);
  return true;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4IPRewriter)
