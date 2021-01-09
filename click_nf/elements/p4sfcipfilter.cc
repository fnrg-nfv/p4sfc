
// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "p4sfcipfilter.hh"
#include "p4header.hh"
#include <click/error.hh>
#include <click/args.hh>
#include <click/straccum.hh>
#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/icmp.h>
#include <click/integers.hh>
#include <click/etheraddress.hh>
#include <click/nameinfo.hh>
#include <click/error.hh>
#include <click/glue.hh>

#include <iostream>
CLICK_DECLS

static void
separate_text(const String &text, Vector<String> &words)
{
    const char *s = text.data();
    int len = text.length();
    int pos = 0;
    while (pos < len)
    {
        while (pos < len && isspace((unsigned char)s[pos]))
            pos++;
        switch (s[pos])
        {
        case '&':
        case '|':
            if (pos < len - 1 && s[pos + 1] == s[pos])
                goto two_char;
            goto one_char;

        case '<':
        case '>':
        case '!':
        case '=':
            if (pos < len - 1 && s[pos + 1] == '=')
                goto two_char;
            goto one_char;

        case '(':
        case ')':
        case '[':
        case ']':
        case ',':
        case ';':
        case '?':
        one_char:
            words.push_back(text.substring(pos, 1));
            pos++;
            break;

        two_char:
            words.push_back(text.substring(pos, 2));
            pos += 2;
            break;

        default:
        {
            int first = pos;
            while (pos < len && (isalnum((unsigned char)s[pos]) || s[pos] == '-' || s[pos] == '.' || s[pos] == '/' || s[pos] == '@' || s[pos] == '_' || s[pos] == ':'))
                pos++;
            if (pos == first)
                pos++;
            words.push_back(text.substring(first, pos - first));
            break;
        }
        }
    }
}

P4SFCIPFilter::P4SFCIPFilter()
{
    in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

P4SFCIPFilter::~P4SFCIPFilter()
{
    P4SFCState::shutdownServer();
}

int P4SFCIPFilter::configure(Vector<String> &conf, ErrorHandler *errh)
{
    int click_instance_id = 1;
    _debug = false;
    if (Args(conf, this, errh)
            .read_mp("CLICKINSTANCEID", click_instance_id)
            .read_mp("DEBUG", _debug)
            .consume() < 0)
        return -1;

    P4SFCState::startServer(click_instance_id);

    for (int argno = 2; argno < conf.size(); argno++)
    {
        // parse every rules into statelib
        Vector<String> words;
        separate_text(cp_unquote(conf[argno]), words);

        P4SFCState::TableEntry *e = parse(words, errh);
        // TODO: priority ascending or descending
        e->set_priority(argno);
        _rules.insert(*e);
        if (_debug)
            click_chatter("entry: %s", toString(*e).c_str());
    }
}

struct QuickEntry
{
    IPFlowID flowid;
    IPFlowID mask;
    uint8_t proto;
    uint8_t proto_mask;

    int port;
    bool match(const IPFlow5ID &cmp)
    {
        return (cmp & mask) == flowid && (cmp.proto() & proto_mask) == proto;
    }
};

std::map<P4SFCState::TableEntry *, QuickEntry> quick_map;

P4SFCState::TableEntry *P4SFCIPFilter::parse(Vector<String> &words, ErrorHandler *errh)
{
    QuickEntry ce;
    int size = words.size();
    if (size != 4)
    {
        errh->message("firewall rule must be 4-tuple, but is a %d-tuple", size);
        return NULL;
    }

    P4SFCState::TableEntry *e = P4SFCState::newTableEntry();
    e->set_table_name(P4_IPFILTER_TABLE_NAME);

    int out_port;
    if (!IntArg().parse(words[0], out_port))
    {
        if (words[0].compare("allow") == 0)
            out_port = 0;
        else if (words[0].compare("deny") == 0)
            out_port = -1;
        else
        {
            errh->message("format error %s", words[0]);
            out_port = -1;
        }
    }
    {
        auto a = e->mutable_action();
        a->set_action(P4_IPFILTER_ACTION_NAME);
        auto p = a->add_params();
        p->set_param(P4_IPFILTER_PARAM_PORT);
        p->set_value(&out_port, 4);
    }
    ce.port = out_port;
    P4SFCIPFilter::SingleAddress src = parseSingleAddress(words[1]);
    P4SFCIPFilter::SingleAddress dst = parseSingleAddress(words[2]);
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_SADDR);
        auto t = m->mutable_ternary();
        t->set_value(&src.ip, 4);
        t->set_mask(&src.ip_mask, 4);
    }
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_DADDR);
        auto t = m->mutable_ternary();
        t->set_value(&dst.ip, 4);
        t->set_mask(&dst.ip_mask, 4);
    }
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_SPORT);
        auto t = m->mutable_ternary();
        t->set_value(&src.port, 2);
        t->set_mask(&src.port_mask, 2);
    }
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_DPORT);
        auto t = m->mutable_ternary();
        t->set_value(&dst.port, 2);
        t->set_mask(&dst.port_mask, 2);
    }
    ce.flowid.assign(src.ip, src.port, dst.ip, dst.port);
    ce.mask.assign(src.ip_mask, src.port_mask, dst.ip_mask, dst.port_mask);
    {
        uint8_t proto = 0;
        uint8_t proto_mask = 0;
        if (words[3] != "-")
        {
            if (IntArg().parse(words[3], proto))
                proto_mask = 0xff;
            else
                errh->message("bad proto number %s", words[3].c_str());
        }
        auto m = e->add_match();
        m->set_field_name(P4H_IP_PROTO);
        auto t = m->mutable_ternary();
        t->set_value(&proto, 1);
        t->set_mask(&proto_mask, 1);
        ce.proto = proto;
        ce.proto_mask = proto_mask;
    }
    quick_map.insert({e, ce});
    return e;
}

P4SFCIPFilter::SingleAddress P4SFCIPFilter::parseSingleAddress(String &word)
{
    P4SFCIPFilter::SingleAddress ret;
    ret.port = 0;
    ret.port_mask = 0;

    if (word == "-")
    {
        ret.ip = 0;
        ret.ip_mask = 0;
    }
    else
    {
        int middle_pos = word.find_left(":");

        String ip_str;
        String port_str = nullptr;
        if (middle_pos == -1)
            ip_str = word;
        else
        {
            ip_str = word.substring(0, middle_pos);
            port_str = word.substring(middle_pos + 1);
        }

        IPAddress ip;
        IPAddress mask;
        bool result = IPPrefixArg().parse(ip_str, ip, mask);
        ip &= mask;
        ret.ip = ip.addr();
        ret.ip_mask = mask.addr();

        if (port_str)
        {
            int port;
            if (IntArg().parse(port_str, port))
            {
                ret.port = htons(port);
                ret.port_mask = 0xffff;
            }
        }
    }
    return ret;
}

template <class T>
inline T P4SFCIPFilter::s2i(const std::string &s)
{
    return *(T *)s.data();
}

int P4SFCIPFilter::process(int port, Packet *p)
{
    IPFlow5ID flowid(p);
    int out = -1;
    auto lambda = [this, flowid, &out](P4SFCState::TableEntry *e) mutable {
        auto ce = quick_map.find(e)->second;
        if (ce.match(flowid))
        {
            out = ce.port;
            return true;
        }
        return false;
    };
    auto e = _rules.lookup(lambda);
    if (_debug)
        click_chatter("Filter the packet to port %x.", out);
    return out;
}

void P4SFCIPFilter::push(int port, Packet *p)
{
    checked_output_push(process(port, p), p);
}

void P4SFCIPFilter::push_batch(int port, PacketBatch *batch)
{
    auto fnt = [this, port](Packet *p) { return process(port, p); };
    CLASSIFY_EACH_PACKET(noutputs() + 1, fnt, batch, checked_output_push_batch);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCIPFilter)
