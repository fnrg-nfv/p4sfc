#include <click/config.h>
#include "p4sfcsimuflow.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <click/straccum.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/etheraddress.hh>

#include <arpa/inet.h>
CLICK_DECLS

const unsigned P4SFCSimuFlow::NO_LIMIT;
const unsigned P4SFCSimuFlow::DEF_BATCH_SIZE;

struct xorshift128p_state
{
    uint64_t a, b;
};

static struct xorshift128p_state state;

/* The state must be seeded so that it is not all zero */
inline uint64_t xorshift128p(struct xorshift128p_state &state)
{
    uint64_t t = state.a;
    uint64_t const s = state.b;
    state.a = s;
    t ^= t << 23;       // a
    t ^= t >> 17;       // b
    t ^= s ^ (s >> 26); // c
    state.b = t;
    return t + s;
}

inline uint32_t qrand()
{
    return (uint32_t)xorshift128p(state) % RAND_MAX;
}

P4SFCSimuFlow::P4SFCSimuFlow() : _task(this), _timer(&_task)
{
#if HAVE_BATCH
    in_batch_mode = BATCH_MODE_YES;
    _batch_size = DEF_BATCH_SIZE;
#endif
}

int P4SFCSimuFlow::configure(Vector<String> &conf, ErrorHandler *errh)
{
    int rate = 10;
    unsigned burst = 32;
    int limit = -1;
    unsigned eth_type = 0x1234;

    double major_flow = 0.1;
    double major_data = 0.9;

    // default value
    _active = true, _stop = false, _debug = false;
    _len = 64, _range = 0, _flowsize = 100, _count = 0;
    _sipaddr.s_addr = 0x0100000A;
    _dipaddr.s_addr = 0x4D4D4D4D;
    _seed = 0xABCDABCD;
    String sfch = "";

    if (Args(conf, this, errh)
            .read_mp("SRCETH", EtherAddressArg(), _ethh.ether_shost)
            .read_mp("DSTETH", EtherAddressArg(), _ethh.ether_dhost)
            .read("DEBUG", _debug)
            .read("SRCIP", _sipaddr)
            .read("DSTIP", _dipaddr)
            .read("RANGE", _range)
            .read("FLOWSIZE", _flowsize)
            .read("SEED", _seed)
            .read("SFCH", sfch)
            .read("ETHTYPE", eth_type)
            .read("RATE", rate)
            .read("BURST", burst)
            .read("LIMIT", limit)
            .read("ACTIVE", _active)
            .read("LENGTH", _len)
            .read("STOP", _stop)
            .read("MAJORFLOW", major_flow)
            .read("MAJORDATA", major_data)
            .complete() < 0)
        return -1;

    _ethh.ether_type = htons(eth_type);

    if (rate < 0)
        _rate_limit = false;
    else
    {
        _rate_limit = true;
        if (rate < burst)
            burst = rate;
        _tb.assign(rate, burst);
    }

    if (burst < (int)_batch_size)
        _batch_size = burst;

    _limit = (limit >= 0 ? unsigned(limit) : NO_LIMIT);
    _major_data = (major_data >= 0 ? major_data <= 1 ? major_data : .9 : .1) * RAND_MAX;
    major_flow = major_flow >= 0 ? major_flow <= 1 ? major_flow : .9 : .1;
    _major_flowsize = _flowsize * major_flow;

    if (_major_flowsize == 0 && major_data == 1)
        errh->error("major flow is 0 but data is 1");
    else if (_major_flowsize == _flowsize && major_data == 0)
        errh->error("major flow is 1 but data is 0");
    _sfch = sfch;

    // for debug
    if (_debug)
    {
        char buffer[20];
        inet_ntop(AF_INET, &_sipaddr, buffer, sizeof(buffer));
        errh->debug("sipaddr: %s\n", buffer);
        inet_ntop(AF_INET, &_dipaddr, buffer, sizeof(buffer));
        errh->debug("dipaddr: %s\n", buffer);
        errh->debug("range: %d\tflowsize: %d\tseed: %u\tbatchsize: %u\n", _range, _flowsize, _seed, _batch_size);
        errh->debug("major data: %u\n", _major_data);
        errh->debug("major flow size: %u\n", _major_flowsize);
        errh->debug("sfch(%d): %s\n", _sfch.length(), _sfch.c_str());
    }

    return 0;
}

int P4SFCSimuFlow::initialize(ErrorHandler *errh)
{
    click_srandom(_seed);
    state.a = (uint64_t)_seed << 32 | _seed;
    state.b = (uint64_t)_seed << 16 + _seed;
    setup_packets(errh);

    if (output_is_push(0))
        ScheduleInfo::initialize_task(this, &_task, errh);

    _tb.set(_batch_size);

    _timer.initialize(this);

    return 0;
}

void P4SFCSimuFlow::cleanup(CleanupStage)
{
    for (size_t i = 0; i < _flowsize; i++)
    {
        if (_flows[i].packet)
            _flows[i].packet->kill();
        _flows[i].packet = 0;
    }
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

    PacketBatch *head = 0;
    Packet *last;
    unsigned n = _batch_size;
    if (_limit != NO_LIMIT && n + _count >= _limit)
        n = _limit - _count;

    if (!_rate_limit)
    {
        for (int i = 0; i < n; i++)
        {
            Packet *p = next_packet()->clone();
            p->set_timestamp_anno(Timestamp::now());

            if (head == NULL)
                head = PacketBatch::start_head(p);
            else
                last->set_next(p);
            last = p;
        }
        output_push_batch(0, head->make_tail(last, n));
        _count += n;

        _task.fast_reschedule();
        return true;
    }
    else
    {
        // Refill the token bucket
        _tb.refill();

        // Create a batch
        for (int i = 0; i < (int)n; i++)
        {
            if (_tb.remove_if(1))
            {
                Packet *p = next_packet()->clone();
                p->set_timestamp_anno(Timestamp::now());

                if (head == NULL)
                    head = PacketBatch::start_head(p);
                else
                    last->set_next(p);
                last = p;
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
            output_push_batch(0, head->make_tail(last, n));
            if (_debug)
                print_cnt();

            _count += n;

            _task.fast_reschedule();
            return true;
        }
        else
        {
            _timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(1)));
            return false;
        }
    }
}

void P4SFCSimuFlow::setup_packets(ErrorHandler *errh)
{
    _flows = new flow_t[_flowsize];
    for (unsigned i = 0; i < _flowsize; i++)
    {
        WritablePacket *q = Packet::make(_len);
        _flows[i].packet = q;
        unsigned char *data = q->data();
        memcpy(data, &_ethh, 14);
        data += 14;
        memcpy(data, _sfch.data(), _sfch.length());
        click_ip *ip = reinterpret_cast<click_ip *>(data + _sfch.length());
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

        if (_debug)
            errh->debug("Flow %d: %s:%u -> %s:%u\n", i, IPAddress(ip->ip_src).unparse().c_str(), ntohs(udp->uh_sport),
                        IPAddress(ip->ip_dst).unparse().c_str(), ntohs(udp->uh_dport));
    }
}

inline Packet *P4SFCSimuFlow::next_packet()
{
    unsigned next;

    // click_random() not efficient
    const uint64_t rand = xorshift128p(state);
    const uint32_t rand1 = rand % RAND_MAX;
    const uint32_t rand2 = (rand >> 32) % RAND_MAX;
    if (_major_flowsize == 0)
        next = rand2 % _flowsize;
    else if (rand1 <= _major_data)
        next = rand2 % _major_flowsize;
    else
        next = (rand2 % (_flowsize - _major_flowsize)) + _major_flowsize;
    _flows[next].flow_count++;

    return _flows[next].packet;
}

void P4SFCSimuFlow::print_cnt()
{
    for (size_t i = 0; i < _flowsize; i++)
        printf("%u\t", _flows[i].flow_count);
    printf("\n");
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCSimuFlow)
