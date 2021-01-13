
// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "p4sfcipfilter.hh"
#include "parserhelper.hh"
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
    int port = 28282;
    if (Args(conf, this, errh)
            .read_mp("CLICKINSTANCEID", click_instance_id)
            .read_mp("DEBUG", _debug)
            .read_mp("PORT", port)
            .consume() < 0)
        return -1;

    char addr[20];
    sprintf(addr, "0.0.0.0:%d", port);
    P4SFCState::startServer(click_instance_id, std::string(addr));

    for (int argno = 3; argno < conf.size(); argno++)
    {
        // parse every rules into statelib
        Vector<String> words;
        separate_text(cp_unquote(conf[argno]), words);

        P4SFCState::TableEntry *e = parse(words, errh);
        // TODO: priority ascending or descending
        e->set_priority(argno);
        _map.insert(*e);
        if (_debug)
            click_chatter("entry: %s", toString(*e).c_str());
    }
}

static std::string buildkey(const IPFlow5ID &flow)
{
    std::string ret;
    ret.append((const char *)flow.saddr().data(), 4);
    ret.append((const char *)flow.daddr().data(), 4);
    uint8_t proto = flow.proto();
    ret.append((const char *)&proto, 1);
    return ret;
}

P4SFCState::TableEntry *P4SFCIPFilter::parse(Vector<String> &words, ErrorHandler *errh)
{
    int size = words.size();
    if (size != 4)
    {
        errh->message("firewall rule must be 4-tuple, but is a %d-tuple", size);
        return NULL;
    }

    P4SFCState::TableEntry *e = P4SFCState::newTableEntry();
    e->set_table_name(P4_IPFILTER_TABLE_NAME);

    bool allowed = false;
    if (words[0].compare("allow") == 0)
        allowed = true;
    else if (words[0].compare("deny") == 0)
        allowed = false;
    else
        errh->message("format error %s", words[0]);
    {
        auto a = e->mutable_action();
        if (allowed)
            a->set_action(P4_IPFILTER_ACTION_ALLOW_NAME);
        else
            a->set_action(P4_IPFILTER_ACTION_DENY_NAME);
    }
    IPAddress src;
    IPAddress dst;

    if (!IPAddressArg().parse(words[1], src, this))
        errh->message("src IPAddress: parse error");
    if (!IPAddressArg().parse(words[2], dst, this))
        errh->message("dst IPAddress: parse error");
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_SADDR);
        auto t = m->mutable_exact();
        uint32_t src_addr = src.addr();
        t->set_value(&src_addr, 4);
    }
    {
        auto m = e->add_match();
        m->set_field_name(P4H_IP_DADDR);
        auto t = m->mutable_exact();
        uint32_t dst_addr = dst.addr();
        t->set_value(&dst_addr, 4);
    }
    {
        uint8_t proto = 0;
        if (!IntArg().parse(words[3], proto))
            errh->message("bad proto number %s", words[3].c_str());
        auto m = e->add_match();
        m->set_field_name(P4H_IP_PROTO);
        auto t = m->mutable_exact();
        t->set_value(&proto, 1);
    }
    return e;
}

int P4SFCIPFilter::process(int port, Packet *p)
{
    IPFlow5ID flowid(p);
    int out = -1;
    P4SFCState::TableEntry *e = _map.lookup(buildkey(flowid));
    if (e)
        out = (e->action().action().compare(P4_IPFILTER_ACTION_ALLOW_NAME) == 0);
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
