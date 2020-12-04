#include "p4sfcstate.hh"

#include <iostream>
#include <thread>
#include <chrono>

using namespace P4SFCState;
using namespace std;


void insertOneEntry(Table* t) {
    auto e = newTableEntry();
    e->set_table_name("ipv4_fwd");
    {
        auto m = e->add_match();
        m->set_field_name("hdr.ipv4.dstAddr");
        auto exact = m->mutable_exact();
        exact->set_value("10.0.0.1");
    }
    static int port = 6666;
    {
        auto m = e->add_match();
        m->set_field_name("hdr.ipv4.dstPort");
        auto exact = m->mutable_exact();
        exact->set_value(to_string(port++));
    }
    auto a = e->mutable_action();
    a->set_action("port_fwd");
    {
        auto p = a->add_params();
        p->set_param("port");
        p->set_value("1");
    }
    e->set_priority(0);
    t->insert(e);
    t->lookup(e);
}

int main(int argc, char const *argv[])
{
    init();
    Table t;
    // auto t = new Table();
    for (size_t i = 0; i < 100; i++) 
        insertOneEntry(&t);
    
    cout << "Table size: " << t.size() << endl;
    // this_thread::sleep_for(std::chrono::seconds(1));
    // for(auto i = tables.begin(); i != tables.cend(); i++) {
    //     Table* table = *i;
    //     for (auto j = table->_map.begin(); j != table->_map.cend(); j++)
    //         cout << toString(&j->second) << endl;
    // }
    cout << "Wait for server joining......" << endl;
    end();
    return 0;
}
