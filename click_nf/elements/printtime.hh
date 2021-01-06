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
    const char *port_count() const { return "1/1"; }

    int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
    Packet *simple_action(Packet *p);
protected:
    String _label;
};

CLICK_ENDDECLS
#endif
