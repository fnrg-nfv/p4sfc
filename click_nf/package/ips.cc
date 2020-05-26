// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "ips.hh"
#include <click/error.hh>
#include <iostream>
CLICK_DECLS

SampleIPS::SampleIPS() {}

SampleIPS::~SampleIPS() {}

void SampleIPS::parse_pattern(String &s) {
  char *data = (char *)malloc(s.length() / 2);
  uint32_t cur_p = 0;
  unsigned char cur_v = 0;

  bool first = true;
  for (int i = 0; i < s.length(); ++i) {
    char c = s.at(i);

    if (c >= '0' && c <= '9')
      c -= '0';
    else if (c >= 'a' && c <= 'f')
      c -= 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      c -= 'A' + 10;
    else
      continue;

    if (first) {
      cur_v = c, first = false;
    } else {
      data[cur_p++] = (cur_v << 4) + c;
      cur_v = 0, first = true;
    }
  }

  if (!first)
    data[cur_p++] = cur_v;

  patterns.push_back({.len = cur_p, .data = (char *)realloc(data, cur_p)});
}

#define PATTERN_MAX_LENGTH 100
int SampleIPS::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (conf.size() == 0)
    errh->warning("empty configuration");

  for (int i = 0; i < conf.size(); ++i) {
    String s = conf[i];
    parse_pattern(s);
  }
  print_patterns();
  return 0;
}

void SampleIPS::print_patterns(void) {
  printf("Patterns:\n");
  for (int i = 0; i < patterns.size(); ++i) {
    const uint32_t *data = (const uint32_t *)patterns[i].data;
    for (uint j = 0; j < patterns[i].len / sizeof(uint32_t); ++j) {
      printf("%08x ", ntohl(data[j]));
    }
    printf("\n");
  }
}

void SampleIPS::push(int, Packet *p) {
  for (int i = 0; i < patterns.size(); ++i) {
    if (pattern_match(patterns[i], p)) {
      // TODO: alert something here.
      output(0).push(p);
      return;
    }
  }
  output(1).push(p);
}

bool SampleIPS::pattern_match(IPSPattern &pt, Packet *p) {
  uint32_t len = p->length();
  const unsigned char *d = p->data();

  // naive
  for (uint i = 0; i < len; i++) {
    uint k = 0;
    for (uint j = 0; j < pt.len; j++) {
      if (d[i + j] == pt.data[j]) {
        k++;
      } else
        break;
    }
    if (k == pt.len)
      return true;
  }
  return false;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPS)
