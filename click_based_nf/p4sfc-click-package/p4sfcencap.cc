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

int P4SFCEncap::initialize(ErrorHandler *) {
  std::cout << "MJT TEST: " << std::endl;
  std::cout << sizeof(p4sfc_header_t) << std::endl;
  std::cout << sizeof(nf_header_t) << std::endl;

  nf_header_t nf;
  nf.nfId = 0x7fff;
  nf.isLast = 0;
  printf("%x\n", *reinterpret_cast<uint16_t *>(&nf));

  p4sfc_header_t sfc;
  sfc.chainId = 0;
  sfc.chainLength = 3;
  printf("%x\n", *reinterpret_cast<uint32_t *>(&sfc));
}

void P4SFCEncap::push(int input, Packet *p) {
  p4sfc_header_t *p4sfc_header =
      (p4sfc_header_t *)malloc(sizeof(p4sfc_header_t));
  extract_p4sfc_header(p, p4sfc_header);
  // TODO:

  output(input).push(p);
}

void P4SFCEncap::extract_p4sfc_header(Packet *p, p4sfc_header_t *p4sfc_header) {
  const unsigned char *pdata = p->data();
  uint16_t id = ntohs(*(const uint16_t *)pdata);
  uint16_t size = ntohs(*(const uint16_t *)(pdata + 2));

  for (int i = 0; i < size; i++) {
    nf_header_t *nf = (nf_header_t *)malloc(sizeof(nf_header_t));
    uint16_t id = ntohs(*(const uint16_t *)pdata + 3 + 2 * i);
    // uint16_t = ntohs(*(const uint16_t *)(pdata + 2));
  }

  //   printf("%d %d\n", id, len);
  //   p4sfc_header.chainId = id;

  // for (int i = 0; i < p4sfc_header->chainLength; i++)
  //   printf("%x %d %d\n", *reinterpret_cast<uint16_t
  //   *>(&p4sfc_header->nfs[i]),
  //          p4sfc_header->nfs[i].nfId, p4sfc_header->nfs[i].isLast);
}

unsigned char *P4SFCEncap::encode_p4sfc_header(p4sfc_header_t *) {}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCEncap)
ELEMENT_MT_SAFE(P4SFCEncap)
