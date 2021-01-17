#include <click/config.h>
#include "printtime.hh"
#include <click/error.hh>
#include <click/args.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
CLICK_DECLS

PrintTime::PrintTime() {}

PrintTime::~PrintTime() {}

int PrintTime::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _debug = false;
    _header_offset = 4;
    if (Args(conf, this, errh)
            .read("DEBUG", _debug)
            .read("OFFSET", _header_offset)
            .complete() < 0)
        return -1;

    if (_header_offset > 100)
        errh->warning("_header_offset (%d) is too large\n", _header_offset);

    return 0;
}

String PrintTime::read_handler(Element *e, void *thunk)
{
    PrintTime *fr = static_cast<PrintTime *>(e);

    if (thunk == (void *)0)
    {
        Timestamp avg_lat(0);
        if (fr->_cnt > 0)
            avg_lat = fr->_latency / fr->_cnt;

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
PacketBatch *PrintTime::simple_action_batch(PacketBatch *batch)
{
    FOR_EACH_PACKET_SAFE(batch, p)
    {
        simple_action(p);
    }
    return batch;
}

Packet *PrintTime::simple_action(Packet *p)
{
    const Timestamp *send_time = (const Timestamp *)(p->data() + 14 + _header_offset + sizeof(click_ip) + sizeof(click_udp));
    Timestamp interval = Timestamp::now() - *send_time;

    if (_debug)
        click_chatter("Packet send time: %s\t recv time: %s \tinterval: %s",
                      send_time->unparse().c_str(),
                      Timestamp::now().unparse().c_str(),
                      interval.unparse_interval().c_str());

    if (interval > 0)
    {
        _cnt++;
        _latency += interval;
    }

    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(PrintTime)
