#include <click/config.h>
#include "printtime.hh"
#include <click/error.hh>
#include <click/args.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
CLICK_DECLS

PrintTime::PrintTime() {}

PrintTime::~PrintTime() {}

String PrintTime::read_handler(Element *e, void *thunk)
{
    PrintTime *fr = static_cast<PrintTime *>(e);

    if (thunk == (void *)0)
    {
        Timestamp avg_lat = fr->_latency / fr->_cnt;
        String ret = avg_lat.unparse_interval() + "(" + String(fr->_cnt) + ")";

        fr->_latency = Timestamp();
        fr->_cnt = 0;

        return ret;
    }
    return "<error>";
}

void PrintTime::add_handlers()
{
    add_read_handler("avg_latency", read_handler, 0);
}

Packet *PrintTime::simple_action(Packet *p)
{
    Timestamp interval = Timestamp::now() - p->timestamp_anno();
    if (interval < 0)
    {
        click_chatter("warning interval %s", interval.unparse_interval());
    }
    else
    {
        _cnt++;
        _latency += interval;
    }

    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(PrintTime)
