#include <click/config.h>

#include "printtime.hh"
#include <click/error.hh>
#include <click/args.hh>
CLICK_DECLS

PrintTime::PrintTime() {}

PrintTime::~PrintTime() {}

int PrintTime::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String label = "Time";
    if (Args(conf, this, errh)
            .read_p("LABEL", label)
            .complete() < 0)
        return -1;
    _label = label;
}

Packet *PrintTime::simple_action(Packet *p)
{
    click_chatter("%s: %s", _label, Timestamp::now().unparse().c_str());
    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(PrintTime)
