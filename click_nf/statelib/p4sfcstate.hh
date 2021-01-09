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
        Table() {}
        int size();

        void insert(const string &, TableEntry &);
        void insert(TableEntry &);
        void remove(const string &);
        TableEntry *lookup(const string &);
        TableEntry *lookup(const TableEntry &);

        map<string, TableEntry *> _map;
    };

    class List
    {
    public:
        List() {}
        int size();

        void insert(TableEntry &);
        TableEntry *at(int);
        TableEntry *operator[](int);

        // TODO: std::function is cost heavy
        // TableEntry *lookup(std::function<bool(TableEntry *)> match);
        template <typename T>
        TableEntry *lookup(T match);

        // TODO
        void remove(int index);

    private:
        vector<TableEntry *> _list;
    };

    string buildKey(const TableEntry &entry);
    TableEntry *newTableEntry();
    void deleteTableEntries();
    void startServer(int click_instance_id = 1, string addr = "0.0.0.0:28282");
    void shutdownServer();
    string toString(const TableEntry &entry);

    // should not be exploded
    void incSlot(TableEntry *);

    // inline definition
    inline int Table::size()
    {
        return _map.size();
    }

    inline void
    Table::insert(const string &key, TableEntry &entry)
    {
        _map.insert({key, &entry});
    }

    inline void Table::insert(TableEntry &entry)
    {
        string key = buildKey(entry);
        _map.insert({key, &entry});
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

    inline int List::size()
    {
        return _list.size();
    }

    inline void List::insert(TableEntry &e)
    {
        _list.push_back(&e);
    }
    inline TableEntry *List::at(int i)
    {
        return _list[i];
    }

    inline TableEntry *List::operator[](int i)
    {
        return at(i);
    }

    template <typename T>
    inline TableEntry *List::lookup(T match)
    {
        for (auto e = _list.begin(); e != _list.end(); e++)
            if (match(*e))
            {
                incSlot(*e);
                return *e;
            }
        return 0;
    }

} // namespace P4SFCState

#endif
