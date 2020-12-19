#ifndef P4IPREWRITER_HH
#define P4IPREWRITER_HH
#include <click/bitvector.hh>
#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/ipflowid.hh>
#include <click/timer.hh>

#include "p4sfcstate.hh"

CLICK_DECLS

/*
 * configure(id, [input_specs+])
 */

class P4IPRewriterInput;
class P4IPRewriterEntry;
class P4IPRewriter;

class P4Element : public Element {
// protected:
//   int _click_instance_id;
};

class P4IPRewriterPattern {
public:
  P4IPRewriterPattern(const IPAddress &saddr, int sport, const IPAddress &daddr,
                      int dport, bool sequential, bool same_first,
                      uint32_t variation);
  static bool parse_with_ports(const String &str, P4IPRewriterInput *input,
                               Element *context, ErrorHandler *errh);

  void use() { _refcount++; }
  void unuse() {
    if (--_refcount <= 0)
      delete this;
  }

  operator bool() const { return _saddr || _sport || _daddr || _dport; }
  IPAddress daddr() const { return _daddr; }

  int rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid);

  void unparse(StringAccum &sa) const;

private:
  IPAddress _saddr;
  int _sport; // net byte order
  IPAddress _daddr;
  int _dport; // net byte order

  uint32_t _variation_top;
  uint32_t _next_variation;

  bool _sequential;
  bool _same_first;

  int _refcount;
};

class P4IPRewriterInput {
public:
  enum { i_drop, i_pattern };
  P4IPRewriter *owner;
  int kind;
  uint32_t count;
  uint32_t failures;

  // pattern related vars
  int foutput;
  int routput;
  P4IPRewriterPattern *pattern;

  P4IPRewriterInput();

  int rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid);

  void unparse(StringAccum &sa) const;
};

class P4IPRewriter : public P4Element {
public:
  enum { rw_drop = -1, rw_addmap = -2 }; // rw result

  P4IPRewriter();
  ~P4IPRewriter();

  const char *class_name() const { return "P4IPRewriter"; }
  const char *port_count() const { return "1-/1-"; }
  const char *processing() const { return PUSH; }
  const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  // inline P4IPRewriterEntry *get_entry(const IPFlowID &flowid, int input);
  P4SFCState::TableEntry *add_flow(const IPFlowID &flowid, const IPFlowID &rewritten_flowid, int input);
  // void destroy_flow(P4IPRewriterFlow *flow);

  void push(int, Packet *);


protected:
  // HashContainer<P4IPRewriterEntry> _map;
  P4SFCState::Table _map;
  Vector<P4IPRewriterInput> _input_specs;

private:
  int parse_input_spec(const String &line, P4IPRewriterInput &is,
                       int input_number, ErrorHandler *errh);
};

CLICK_ENDDECLS
#endif
