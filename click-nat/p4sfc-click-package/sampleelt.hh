#ifndef SAMPLEPACKAGEELEMENT_HH
#define SAMPLEPACKAGEELEMENT_HH
#include "libswitch.h"
#include <click/element.hh>
CLICK_DECLS

/*
 * =c
 * SamplePackageElement()
 * =s debugging
 * demonstrates how to write a package
 * =d
 *
 * This is the only element in the `sample' package. It demonstrates how to
 * write an element that will be placed in a package. It does nothing except
 * report that the package was successfully loaded when it initializes. */

class SamplePackageElement : public Element {

public:
  SamplePackageElement();
  ~SamplePackageElement();

  const char *class_name() const { return "SamplePackageElement"; }
  const char *port_count() const { return "1/1"; }
  const char *processing() const { return "a/a"; }
  const char *flow_code() const { return "x/y"; }

  int initialize(ErrorHandler *errh);
  Packet *simple_action(Packet *);

private:
  int count;
};

CLICK_ENDDECLS
#endif
