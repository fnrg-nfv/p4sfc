#include "p4sfcstate.hh"

#include <iostream>
using namespace P4SFCState;
using namespace std;


void insertOneEntry(Table* t) {
    auto e = new TableEntry(); 
    e->set_table_name("ipv4_fwd");
    {
        
        auto m = e->add_match();
        m->set_field_name("hdr.ipv4.dstAddr");
        auto exact = m->exact();
        exact.set_value("10.0.0.1");
    }

    {
        auto m = e->add_match();
        m->set_field_name("hdr.ipv4.dstPort");
        auto exact = m->exact();
        exact.set_value("6666");
    }

    t->insert(e);
}

int main(int argc, char const *argv[])
{
    /* code */
    init();
    auto t = new Table();
    insertOneEntry(t);
    cout << t->size() << endl;
    while (true) {}
    return 0;
}
