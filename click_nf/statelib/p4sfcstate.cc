#include <thread>
#include <grpc++/grpc++.h>

#ifdef DEBUG
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#endif

#include "p4sfcstate.hh"

#include "p4sfcstate.grpc.pb.h"

namespace P4SFCState
{

    using namespace grpc;

    vector<TableEntry *> total_entries;
    int cur_pos;
    const int k = 1000;

    // Logic and data behind the server's behavior.
    class ServiceImpl final : public RPC::Service
    {
        Status SayHello(ServerContext *context, const HelloRequest *request,
                        HelloReply *reply) override
        {
            std::string prefix("Hello ");
            reply->set_message(prefix + request->name());
            return Status::OK;
        }

        typedef struct
        {
            TableEntry *e;
            uint64_t sum;
        } entry_sum;

        Status GetState(ServerContext *context, const Empty *request, TableEntryReply *reply)
        {
#ifdef DEBUG
            auto t_start = std::chrono::high_resolution_clock::now();
#endif
            reply->set_window_cur_pos(cur_pos);
            reply->set_click_instance_id(_click_instance_id);

            size_t size = total_entries.size();
            vector<entry_sum> entries;
            for (auto i = total_entries.begin(); i != total_entries.cend(); i++)
            {
                TableEntry *e = *i;
                entries.push_back({e, window_sum(*e)});
            }

            std::sort(entries.begin(), entries.end(),
                      [this](const entry_sum &a, const entry_sum &b) {
                          return a.sum > b.sum;
                      });

            size = k < size ? k : size;
            for (size_t i = 0; i < size; i++)
                reply->add_entries()->CopyFrom(*(entries[i].e));

            move_window_forward();

#ifdef DEBUG
            std::cout << "Get size:" << size << "/" << total_entries.size() << std::endl;
            auto t_end = std::chrono::high_resolution_clock::now();
            std::cout << std::fixed << std::setprecision(2)
                      << "Get state time passed:"
                      << std::chrono::duration<double, std::milli>(t_end - t_start).count() << " ms\n";
#endif
            return Status::OK;
        }

    public:
        ServiceImpl(int click_instance_id) : _click_instance_id(click_instance_id) {}

    private:
        int _click_instance_id;

        void move_window_forward()
        {
            int nextPos = (cur_pos + 1) % WINDOW_SIZE;
            for (auto i = total_entries.begin(); i != total_entries.cend(); i++)
            {
                TableEntry *e = *i;
                auto w = e->mutable_window();
                w->set_slot(nextPos, w->slot(cur_pos));
            }
            cur_pos = nextPos;
        }

        uint64_t window_sum(const TableEntry &e)
        {
            auto w = e.window();
            return w.slot(cur_pos) - w.slot((cur_pos + 1) % WINDOW_SIZE);
        }
    };

    std::unique_ptr<Server> server;
    thread *server_thread;

    void runServer(string addr, int click_instance_id)
    {
        cur_pos = 0;

        ServiceImpl service(click_instance_id);
        ServerBuilder builder;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = builder.BuildAndStart();

        std::cout << "Server listening on " << addr << std::endl;
        server->Wait();
    }

    void startServer(int click_instance_id, string addr)
    {
        if (server_thread)
            return;
        server_thread = new thread(runServer, addr, click_instance_id);
    }

    void shutdownServer()
    {
        deleteTableEntries();
        server->Shutdown();
        server_thread->join();
    }

    string bigint_to_hexstr(const string &s)
    {
        typedef uint8_t unit;
        const unit *i = (const unit *)s.data();
        std::stringstream stream;
        stream << "0x";
        int len = s.length() / sizeof(unit);
        for (size_t j = 0; j < len; j++)
        {
            stream << std::setfill('0') << std::setw(sizeof(unit) * 2)
                   << std::hex << unsigned(i[j]);
        }
        return stream.str();
    }

    string
    toString(const TableEntry &entry)
    {
        string ret("");
        std::stringstream stream;
        stream << "TableName: " << entry.table_name();
        stream << "\tMatch: ";
        int size = entry.match_size();
        for (size_t i = 0; i < size; i++)
        {
            auto m = entry.match(i);
            stream << m.field_name() << ": ";
            if (m.has_exact())
                stream << "exact: "
                       << bigint_to_hexstr(m.exact().value());
            else if (m.has_ternary())
                stream << "ternary: "
                       << bigint_to_hexstr(m.ternary().value()) << "/"
                       << bigint_to_hexstr(m.ternary().mask());
            //    << int_to_hexstr(m.ternary().value().data()) << "/"
            //    << int_to_hexstr(m.ternary().mask().data());
            stream << "; ";
        }
        auto a = entry.action();
        size = a.params_size();
        stream << "\tAction: " << a.action() << " param: ";
        for (size_t i = 0; i < size; i++)
        {
            auto p = a.params(i);
            stream << p.param() << ": " << p.value() << "; ";
        }
        stream << "\tPriority: " << to_string(entry.priority());
        auto w = entry.window();
        size = w.slot_size();
        stream << "\tSlidingWindow(" + to_string(size) + "): ";
        for (size_t i = 0; i < size; i++)
            stream << to_string(w.slot(i)) << " ";
        return stream.str();
    }

    TableEntry *Table::lookup(const string &key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return 0;
        auto e = it->second;
        incSlot(e);
        return e;
    }

    void incSlot(TableEntry *entry)
    {
        auto w = entry->mutable_window();
        w->set_slot(cur_pos, w->slot(cur_pos) + 1);
    }

    TableEntry *newTableEntry()
    {
        TableEntry *entry = new TableEntry();
        total_entries.push_back(entry);
        // init sliding window
        SlidingWindow *window = entry->mutable_window();
        for (int i = 0; i < WINDOW_SIZE; i++)
            window->add_slot(0);
        return entry;
    }

    void deleteTableEntries()
    {
        for (auto e = total_entries.begin(); e != total_entries.end(); e = total_entries.erase(e))
            if (*e)
                delete *e;
    }

    // concat all val as key, seperated by commas
    // entry maybe a true tableentry or something built from packet header
    string buildKey(const TableEntry &entry)
    {
        string ret("");
        int match_size = entry.match_size();
        for (int i = 0; i < match_size; i++)
        {
            auto m = entry.match(i);
            // TODO: only support exact currently
            if (m.has_exact())
            {
                ret += m.exact().value();
            }
            else if (m.has_ternary())
            {
                ret += m.ternary().value();
                ret += m.ternary().mask();
            }
        }
        return ret;
    }

} // namespace P4SFCState