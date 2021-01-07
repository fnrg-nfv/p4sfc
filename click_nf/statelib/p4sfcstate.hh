#ifndef P4SFCSTATE_HH
#define P4SFCSTATE_HH

#include "p4sfcstate.pb.h"

#include <map>
#include <string>
#include <vector>

#define WINDOW_SIZE 5

namespace P4SFCState
{
    using namespace std;

    // declare
    class Table
    {
    public:
        Table();
        int size();

        void insert(const string &, const TableEntry &);
        void insert(const TableEntry &);
        void remove(const string &);
        TableEntry *lookup(const string &);
        TableEntry *lookup(const TableEntry &);

        map<string, TableEntry> _map;
    };

    string buildKey(const TableEntry &entry);
    TableEntry *newTableEntry();
    void startServer(int click_instance_id = 1, string addr = "0.0.0.0:28282");
    void shutdownServer();
    string toString(const TableEntry &entry);

    // inline definition
    inline int Table::size()
    {
        return _map.size();
    }

    inline void
    Table::insert(const string &key, const TableEntry &entry)
    {
        _map.insert({key, entry});
    }

    inline void Table::insert(const TableEntry &entry)
    {
        string key = buildKey(entry);
        _map.insert({key, entry});
    }

    inline TableEntry *Table::lookup(const TableEntry &e)
    {
        string key = buildKey(e);
        return lookup(key);
    }

    inline void Table::remove(const string &key)
    {
        _map.erase(key);
    }
} // namespace P4SFCState

#endif
