#ifndef P4SFCSTATE_HH
#define P4SFCSTATE_HH

#include "p4sfcstate.pb.h"

#include <map>
#include <string>
#include <vector>

#define WINDOW_SIZE 5

namespace P4SFCState {

using namespace std;

class Table {
    private:
    public:
        map<string, TableEntry> _map;
        Table();
        int size();

        void insert(TableEntry *);
        void remove(TableEntry *);
        TableEntry* lookup(TableEntry *);
};

void init(string addr="localhost:28282");
TableEntry* NewTableEntry();

}

#endif
