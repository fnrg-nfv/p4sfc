#include <click/config.h>
#include "p4sfcsimuflow.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <click/straccum.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/glue.hh>
#include <click/etheraddress.hh>
#include <click/handlercall.hh>

#include <arpa/inet.h>
CLICK_DECLS

const unsigned P4SFCSimuFlow::NO_LIMIT;
const unsigned P4SFCSimuFlow::DEF_BATCH_SIZE;

P4SFCSimuFlow::P4SFCSimuFlow() : _packet(0), _task(this), _timer(&_task)
{
#if HAVE_BATCH
    in_batch_mode = BATCH_MODE_YES;
    _batch_size = DEF_BATCH_SIZE;
#endif
}

int P4SFCSimuFlow::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String data =
        "Random bullshit in a packet, at least 64 bytes long. Well, now it is.";
    unsigned rate = 10;
    unsigned bandwidth = 0;
    int limit = -1;
    int datasize = 64;
    bool active = true, stop = false;
    _headroom = Packet::default_headroom;
    _range = 0;
    _flowsize = 100;
    _ratio = 81;
    _seed = 0;
    _debug = false;

    // default address
    _sipaddr.s_addr = 0x0100000A;
    _dipaddr.s_addr = 0x4D4D4D4D;

    if (Args(conf, this, errh)
            .read_mp("SRCETH", EtherAddressArg(), _ethh.ether_shost)
            .read_mp("DSTETH", EtherAddressArg(), _ethh.ether_dhost)
            .read("DEBUG", _debug)
            .read("SRCIP", _sipaddr)
            .read("DSTIP", _dipaddr)
            .read("RANGE", _range)
            .read("FLOWSIZE", _flowsize)
            .read("SEED", _seed)
            .read("RATIO", _ratio)
            .read("RATE", rate)
            .read("LIMIT", limit)
            .read("ACTIVE", active)
            .read("HEADROOM", _headroom)
            .read("LENGTH", datasize)
            .read("STOP", stop)
            .complete() < 0)
        return -1;

    _ethh.ether_type = htons(0x0800);
    _data = data;
    _len = datasize;

    int burst = rate < 200 ? 2 : rate / 100;

    // for debug
    if (_debug)
    {
        char buffer[20];
        inet_ntop(AF_INET, &_sipaddr, buffer, sizeof(buffer));
        errh->debug("sipaddr: %s\n", buffer);
        inet_ntop(AF_INET, &_dipaddr, buffer, sizeof(buffer));
        errh->debug("dipaddr: %s\n", buffer);
        errh->debug("range: %d\tflowsize: %d\tseed: %d\n", _range, _flowsize, _seed);
    }

#if HAVE_BATCH
    if (burst < (int)_batch_size)
        _batch_size = burst;
#endif

    _tb.assign(rate, burst);
    _limit = (limit >= 0 ? unsigned(limit) : NO_LIMIT);
    _active = active;
    _stop = stop;

    return 0;
}

int P4SFCSimuFlow::initialize(ErrorHandler *errh)
{
    setup_packets();

    _count = 0;
    if (output_is_push(0))
        ScheduleInfo::initialize_task(this, &_task, errh);

#if HAVE_BATCH
    _tb.set(_batch_size);
#else
    _tb.set(1);
#endif

    _timer.initialize(this);

    return 0;
}

void
    P4SFCSimuFlow::cleanup(CleanupStage)
{
    if (_packet)
        _packet->kill();

    _packet = 0;
}

bool P4SFCSimuFlow::run_task(Task *)
{
    if (!_active)
        return false;
    if (_limit != NO_LIMIT && _count >= _limit)
    {
        if (_stop)
            router()->please_stop_driver();
        return false;
    }

    // Refill the token bucket
    _tb.refill();

#if HAVE_BATCH
    PacketBatch *head = 0;
    Packet *last;

    unsigned n = _batch_size;
    unsigned count = 0;

    if (_limit != NO_LIMIT && n + _count >= _limit)
        n = _limit - _count;

    // Create a batch
    for (int i = 0; i < (int)n; i++)
    {
        if (_tb.remove_if(1))
        {
            Packet *p = next_packet()->clone();
            // Packet *p = _packet->clone();
            p->set_timestamp_anno(Timestamp::now());

            if (head == NULL)
            {
                head = PacketBatch::start_head(p);
            }
            else
            {
                last->set_next(p);
            }
            last = p;

            count++;
        }
        else
        {
            _timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(_batch_size)));
            return false;
        }
    }

    // Push the batch
    if (head)
    {
        output_push_batch(0, head->make_tail(last, count));
        _count += count;

        _task.fast_reschedule();
        return true;
    }
    else
    {
        _timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(1)));

        return false;
    }
#else
    if (_tb.remove_if(1))
    {
        Packet *p = _packet->clone();
        p->set_timestamp_anno(Timestamp::now());
        output(0).push(p);
        _count++;
        _task.fast_reschedule();
        return true;
    }
    else
    {
        _timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(1)));

        return false;
    }
#endif
}

void P4SFCSimuFlow::setup_packets()
{
    if (_packet)
        _packet->kill();

    // TOOD: different meaning now
    if (_len < 0)
    {
        _packet = Packet::make(_headroom, _data.data(), _data.length(), 0);
    }
    else if (_len <= _data.length())
    {
        _packet = Packet::make(_headroom, _data.data(), _len, 0);
    }
    else
    {
        // make up some data to fill extra space
        StringAccum sa;
        while (sa.length() < _len)
            sa << _data;
        _packet = Packet::make(_headroom, sa.data(), _len, 0);
    }

    _flows = new flow_t[_flowsize];
    for (unsigned i = 0; i < _flowsize; i++)
    {
        WritablePacket *q = Packet::make(_len);
        _flows[i].packet = q;
        memcpy(q->data(), &_ethh, 14);
        click_ip *ip = reinterpret_cast<click_ip *>(q->data() + 14);
        click_udp *udp = reinterpret_cast<click_udp *>(ip + 1);

        // set up IP header
        ip->ip_v = 4;
        ip->ip_hl = sizeof(click_ip) >> 2;
        ip->ip_len = htons(_len - 14);
        ip->ip_id = 0;
        ip->ip_p = IP_PROTO_UDP;
        ip->ip_src.s_addr = htonl(ntohl(_sipaddr.s_addr) + (click_random() % (_range + 1)));
        ip->ip_dst.s_addr = htonl(ntohl(_dipaddr.s_addr) + (click_random() % (_range + 1)));
        ip->ip_tos = 0;
        ip->ip_off = 0;
        ip->ip_ttl = 250;
        ip->ip_sum = 0;
        ip->ip_sum = click_in_cksum((unsigned char *)ip, sizeof(click_ip));
        _flows[i].packet->set_dst_ip_anno(IPAddress(_dipaddr));
        _flows[i].packet->set_ip_header(ip, sizeof(click_ip));

        // set up UDP header
        udp->uh_sport = (click_random() >> 2) % 0xFFFF;
        udp->uh_dport = (click_random() >> 2) % 0xFFFF;
        udp->uh_sum = 0;
        unsigned short len = _len - 14 - sizeof(click_ip);
        udp->uh_ulen = htons(len);
        udp->uh_sum = 0;
        _flows[i].flow_count = 0;
    }
}

Packet *P4SFCSimuFlow::next_packet()
{
    static int cur = 0;
    cur %= _flowsize;
    return _flows[cur++].packet;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCSimuFlow)
