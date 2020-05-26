#ifndef CLICK_CUSTOMENCAP_HH
#define CLICK_CUSTOMENCAP_HH
#include <click/bitvector.hh>
#include <click/element.hh>
#include <queue>
CLICK_DECLS

class CustomEncap : public Element {
public:
  enum { pull_success, pull_fail };
  CustomEncap() {}
  ~CustomEncap() {}

  const char *class_name() const { return "CustomEncap"; }
  const char *port_count() const { return "1/1"; }
  const char *processing() const { return "a/a"; }
  const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  Packet *simple_action(Packet *);


protected:
  char* _header; 
  int _len;
  
private:
  void parse_pattern(String &s);
};

CLICK_ENDDECLS
#endif
