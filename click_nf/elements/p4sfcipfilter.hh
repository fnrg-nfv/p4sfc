#ifndef CLICK_P4SFCIPFILTER_HH
#define CLICK_P4SFCIPFILTER_HH
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
    struct SingleAddress
    {
        // these are all in network order
        uint32_t ip;
        uint32_t ip_mask;
        uint16_t port;
        uint16_t port_mask;
    };

    P4SFCState::TableEntry *parse(Vector<String> &, ErrorHandler *);
    SingleAddress parseSingleAddress(String &);

    int apply(Packet *, P4SFCState::TableEntry *);
    bool match(Packet *, P4SFCState::TableEntry *);

    template <class T>
    T s2i(const std::string &);

private:
    Vector<P4SFCState::TableEntry *> rules;
};

CLICK_ENDDECLS
#endif
