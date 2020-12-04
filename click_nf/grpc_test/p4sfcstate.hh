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
        map<string, TableEntry> _map; // TODO: should be private. Try not to use it.
        Table();
        int size();

        void insert(TableEntry *);
        void remove(TableEntry *);
        TableEntry* lookup(TableEntry *);
};

void init(string addr="localhost:28282");
void end();
TableEntry* newTableEntry();
string toString(TableEntry *entry);
// extern vector<Table*> tables;
}

#endif
