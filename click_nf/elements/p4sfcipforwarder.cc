
// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "p4sfcipforwarder.hh"
#include "p4header.hh"
#include "parserhelper.hh"
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

P4SFCIPForwarder::P4SFCIPForwarder()
{
    in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

P4SFCIPForwarder::~P4SFCIPForwarder()
{
    P4SFCState::shutdownServer();
}

int P4SFCIPForwarder::configure(Vector<String> &conf, ErrorHandler *errh)
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
    int port;

    bool match(const IPFlow5ID &cmp)
    {
        return (cmp & mask) == flowid;
    }
    QuickEntry(IPAddress dst_ip, IPAddress dst_ip_mask, uint16_t dst_port, uint16_t dst_port_mask, int p)
    {
        flowid.assign(IPAddress(), 0, dst_ip, dst_port);
        mask.assign(IPAddress(), 0, dst_ip_mask, dst_port_mask);
        port = p;
    }
};

static std::map<P4SFCState::TableEntry *, QuickEntry> quick_map;

P4SFCState::TableEntry *P4SFCIPForwarder::parse(Vector<String> &words, ErrorHandler *errh)
{
    int size = words.size();
    if (size != 2)
    {
        errh->message("firewall rule must be 2-tuple, but is a %d-tuple", size);
        return NULL;
    }

    P4SFCState::TableEntry *e = P4SFCState::newTableEntry();
    e->set_table_name(P4_IPFORWARDER_TABLE_NAME);

    int out_port = 0;
    if (!IntArg().parse(words[0], out_port))
    {
        if (!(words[0].compare("allow") == 0))
            errh->message("format error %s", words[0]);
    }
    {
        auto a = e->mutable_action();
        a->set_action(P4_IPFORWARDER_ACTION_NAME);
        auto p = a->add_params();
        p->set_param(P4_IPFORWARDER_PARAM_PORT);
        p->set_value(&out_port, 4);
    }
    SingleAddress dst = SingleAddress::parse(words[1]);
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_DADDR);
        auto t = m->mutable_ternary();
        t->set_value(&dst.ip, 4);
        t->set_mask(&dst.ip_mask, 4);
    }
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_DPORT);
        auto t = m->mutable_ternary();
        t->set_value(&dst.port, 2);
        t->set_mask(&dst.port_mask, 2);
    }
    quick_map.insert({e, QuickEntry(dst.ip, dst.ip_mask, dst.port, dst.port_mask, out_port)});
    return e;
}


int P4SFCIPForwarder::process(int port, Packet *p)
{
    IPFlow5ID flowid(p);
    int out = 0;
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
        click_chatter("Forward the packet(%s) to port %x rule %d.", flowid.unparse().c_str(), out, e->priority());
    return out;
}

void P4SFCIPForwarder::push(int port, Packet *p)
{
    checked_output_push(process(port, p), p);
}

void P4SFCIPForwarder::push_batch(int port, PacketBatch *batch)
{
    auto fnt = [this, port](Packet *p) { return process(port, p); };
    CLASSIFY_EACH_PACKET(noutputs() + 1, fnt, batch, checked_output_push_batch);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCIPForwarder)
