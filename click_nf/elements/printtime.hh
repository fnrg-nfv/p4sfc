#ifndef PRINTTIMEELEMENT_HH
#define PRINTTIME_HH
#include <click/element.hh>
CLICK_DECLS

class PrintTime : public Element
{
public:
    PrintTime();
    ~PrintTime();

    const char *class_name() const { return "PrintTime"; }

    int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
    Packet *simple_action(Packet *p);
protected:
    String _label;
};

CLICK_ENDDECLS
#endif
