/*
 * simpleidle.{cc,hh} -- element sits there and kills packets sans notification
 * Eddie Kohler
 *
 * Copyright (c) 2009 Intel Corporation
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

#include <bitset>
#include <click/config.h>
#include <iostream>

#include "p4sfcencap.hh"

CLICK_DECLS

void P4SFCEncap::push(int input, Packet *p) {
  p4sfc_header_t *psh;
  if (input == 0) {
    psh = (p4sfc_header_t *)malloc(sizeof(p4sfc_header_t));
    int result = pull_p4sfc_header(p, psh);
    if (result == pull_fail) {
      p->kill();
      return;
    }
    _psh_queue.push(psh);
  } else if (input == 1) {
    assert(!_psh_queue.empty());
    psh = _psh_queue.front();
    _psh_queue.pop();
    p = push_p4sfc_header(p, psh);
  }

  output(input).push(p);
}

int P4SFCEncap::pull_p4sfc_header(Packet *p, p4sfc_header_t *psh) {
  const unsigned char *pdata = p->data();
  psh->id = ntohs(*(const uint16_t *)pdata);
  psh->len = ntohs(*(const uint16_t *)(pdata + 2));
  psh->nfs = (nf_header_t *)malloc(sizeof(nf_header_t) * psh->len);
  pdata += P4SFCHEADERSIZE;

  if (psh->len == 0 || psh->len > 100) {
    std::cout << "drop" << std::endl;
    return pull_fail;
  }

  for (int i = 0; i < psh->len; i++, pdata += NFHEADERSIZE) {
    nf_header_t *nf = psh->nfs + i;
    uint16_t data = ntohs(*(const uint16_t *)pdata);
    nf->id = (data >> 1) & 0x7fff;
    nf->isLast = data & 0x1;
  }

  size_t pull_size = pdata - p->data();
  p->pull(pull_size);
  return pull_success;
}

Packet *P4SFCEncap::push_p4sfc_header(Packet *p, p4sfc_header_t *psh) {
  size_t push_size = P4SFCHEADERSIZE + (psh->len - 1) * NFHEADERSIZE;
  WritablePacket *wp = p->push(push_size);
  const unsigned char *pdata = p->data();

  *(uint16_t *)pdata = htons(psh->id);
  pdata += 2;

  *(uint16_t *)pdata = htons(psh->len - 1);
  pdata += 2;

  for (int i = 1; i < psh->len; i++, pdata += NFHEADERSIZE) {
    nf_header_t *nf = psh->nfs + i;
    uint16_t data = (nf->id << 1) | nf->isLast;
    *(uint16_t *)pdata = htons(data);
  }

  return wp;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCEncap)
ELEMENT_MT_SAFE(P4SFCEncap)
