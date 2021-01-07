#ifndef CLICK_P4SFCIPFILTER2_HH
#define CLICK_P4SFCIPFILTER2_HH
#include <click/batchelement.hh>
#include <click/element.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>

#include "/home/sonic/p4sfc/click_nf/statelib/p4sfcstate.hh"

CLICK_DECLS

#define DEBUG

class P4SFCIPFilter : public Element
{
public:
    P4SFCIPFilter();  // SEE sample.cc FOR CONSTRUCTOR
    ~P4SFCIPFilter(); // SEE sample.cc FOR DESTRUCTOR

    const char *class_name() const { return "P4SFCIPFilter"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);

protected:
    P4SFCState::TableEntry *parse(Vector<String> &, ErrorHandler *);
    Vector<P4SFCState::TableEntry *> entries;

    // P4SFCState::Table _table;

    struct SingleAddress
    {
        // these are all in network order
        uint32_t ip;
        uint32_t ip_mask;
        uint16_t port;
        uint16_t port_mask;
    };
    SingleAddress parseSingleAddress(String &);
};

CLICK_ENDDECLS
#endif
