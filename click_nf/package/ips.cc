// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "ips.hh"
#include <click/error.hh>
#include <iostream>
CLICK_DECLS

SampleIPS::SampleIPS() {}

SampleIPS::~SampleIPS() {}

#define PATTERN_MAX_LENGTH 100
int SampleIPS::configure(Vector<String> &conf, ErrorHandler *errh) {

  char buf[PATTERN_MAX_LENGTH];
  for (int i = 0; i < conf.size(); ++i) {
    String s = conf[i];
    
    
    // char *token = strtok(, " ");
    // loop through the string to extract all other tokens
    // while (token != NULL) {
    //   printf(" %s\n", token); // printing each token
    //   token = strtok(NULL, " ");
    // }
    // patterns.push_back(conf[i]);
  }

  for (int i = 0; i < patterns.size(); ++i) {
    // errh->debug(patterns[i].c_str());
  }
  return 0;
}

void SampleIPS::push(int, Packet *p) {
  uint32_t len = p->length();
  const unsigned char *d = p->data();

  for (int i = 0; i < len; i++)
    std::cout << (uint8_t)d[i] << " ";
  std::cout << std::endl;
  output(0).push(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPS)
