#ifndef IPS_HH
#define IPS_HH
#include <click/element.hh>
CLICK_DECLS

class SampleIPS : public Element {
public:
  SampleIPS();
  ~SampleIPS();

  const char *class_name() const { return "SampleIPS"; }
  const char *port_count() const { return "1-/2"; }
  const char *processing() const { return PUSH; }
  const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  void push(int, Packet *);

  struct IPSPattern {
    const uint32_t len;
    const char *data;
  };
  void parse_pattern(String &s);
  bool pattern_match(IPSPattern &pt, Packet *p);

protected:
  Vector<IPSPattern> patterns;
  void print_patterns(void);
};

CLICK_ENDDECLS
#endif
