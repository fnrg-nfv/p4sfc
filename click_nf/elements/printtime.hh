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

    Packet *simple_action(Packet *p);

    int _cnt;
    Timestamp _latency;
    void add_handlers() CLICK_COLD;

private:
    static String read_handler(Element *, void *) CLICK_COLD;
};

CLICK_ENDDECLS
#endif
