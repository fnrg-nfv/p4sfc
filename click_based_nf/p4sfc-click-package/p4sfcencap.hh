#ifndef CLICK_MJTELEMENT_HH
#define CLICK_MJTELEMENT_HH
#include <click/bitvector.hh>
#include <click/element.hh>
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
  uint16_t isLast : 1;
  uint16_t nfId : 15;
} nf_header_t;

typedef struct {
  uint16_t chainId;
  uint16_t chainLength;
  nf_header_t *nfs;
} p4sfc_header_t;

class P4SFCEncap : public Element {
public:
  P4SFCEncap() {}
  ~P4SFCEncap() {}

  const char *class_name() const { return "P4SFCEncap"; }
  const char *port_count() const { return "2/2"; }
  const char *processing() const { return "a/a"; }
  const char *flow_code() const { return "x/y"; }

  void push(int, Packet *);

  int initialize(ErrorHandler *);

private:
  void extract_p4sfc_header(Packet *, p4sfc_header_t *);
  unsigned char *encode_p4sfc_header(p4sfc_header_t *);
};

CLICK_ENDDECLS
#endif
