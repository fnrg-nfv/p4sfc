#ifndef CLICK_P4SFCIPFILTER_HH
#define CLICK_P4SFCIPFILTER_HH
#include <click/batchelement.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>

#include "/home/sonic/p4sfc/click_nf/statelib/p4sfcstate.hh"

CLICK_DECLS

class P4SFCIPFilter : public BatchElement
{
public:
    P4SFCIPFilter();
    ~P4SFCIPFilter();

    const char *class_name() const { return "P4SFCIPFilter"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);
    void push_batch(int port, PacketBatch *batch) override;

protected:
    P4SFCState::TableEntry *parse(Vector<String> &, ErrorHandler *);

    int process(int port, Packet *);
    int apply(P4SFCState::TableEntry *);
    bool match(const IPFlow5ID &, const P4SFCState::TableEntry *);

    template <class T>
    T s2i(const std::string &);

private:
    P4SFCState::Table _map;
    bool _debug;
    // Vector<P4SFCState::TableEntry *> rules;
};

CLICK_ENDDECLS
#endif
