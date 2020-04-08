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
  else if (kind == i_pattern)
    sa << "kind: pattern";
}

bool P4IPRewriterPattern::parse_with_ports(const String &str,
                                           P4IPRewriterInput *input,
                                           Element *context,
                                           ErrorHandler *errh) {
  return false;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4IPRewriter)
