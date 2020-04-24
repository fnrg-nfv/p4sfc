#ifndef CLICK_MJTELEMENT_HH
#define CLICK_MJTELEMENT_HH
#include <click/bitvector.hh>
#include <click/element.hh>
#include <queue>
CLICK_DECLS

/*
 * =c
 * MjtElement
 * =s basicsources
 * discards packets
 * =d
 *
 * Like Idle, but does not provide notification.
 *
 * =sa Idle
 */

typedef struct {
  uint16_t id : 15;
  uint16_t isLast : 1;
} nf_header_t;
#define NFHEADERSIZE 2

typedef struct {
  uint16_t id;
  uint16_t len;
  nf_header_t *nfs;
} p4sfc_header_t;
#define P4SFCHEADERSIZE 4

class P4SFCEncap : public Element {
public:
  enum { pull_success, pull_fail };
  P4SFCEncap() {}
  ~P4SFCEncap() {}

  const char *class_name() const { return "P4SFCEncap"; }
  const char *port_count() const { return "2/2"; }
  const char *processing() const { return "a/a"; }
  const char *flow_code() const { return "x/y"; }

  void push(int, Packet *);

protected:
  std::queue<p4sfc_header_t *> _psh_queue;

private:
  int pull_p4sfc_header(Packet *, p4sfc_header_t *);
  Packet *push_p4sfc_header(Packet *, p4sfc_header_t *);
};

CLICK_ENDDECLS
#endif
