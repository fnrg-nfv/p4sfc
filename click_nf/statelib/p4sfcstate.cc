#include <thread>
#include <grpc++/grpc++.h>

#include "p4sfcstate.hh"

#include "p4sfcstate.grpc.pb.h"

namespace P4SFCState
{

    using namespace grpc;

    vector<Table *> tables;
    int curPos;
    const int k = 10;

    void incSlot(TableEntry *);

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

        Status GetState(ServerContext *context, const Empty *request, TableEntryReply *reply)
        {
            reply->set_click_instance_id(_click_instance_id);
            int size = 0;
            for (auto i = tables.begin(); i != tables.cend(); i++)
            {
                Table *table = *i;
                for (auto j = table->_map.begin(); j != table->_map.cend(); j++)
                {
                    TableEntry *entry = reply->add_entries();
                    entry->CopyFrom(j->second);
                }
            }
            // TODO: need optimize
            std::sort(reply->mutable_entries()->begin(), reply->mutable_entries()->end(),
                      [this](const TableEntry &a, const TableEntry &b) {
                          return window_sum(a) > window_sum(b);
                      });
            // delete more table entries
            move_window_forward();

            return Status::OK;
        }

    public:
        ServiceImpl(int click_instance_id) : _click_instance_id(click_instance_id) {}

    private:
        int _click_instance_id;

        void move_window_forward()
        {
            curPos = (curPos + 1) % WINDOW_SIZE;
            for (auto i = tables.begin(); i != tables.cend(); i++)
            {
                Table *table = *i;
                for (auto j = table->_map.begin(); j != table->_map.cend(); j++)
                    j->second.mutable_window()->set_slot(curPos, 0);
            }
        }

        int window_sum(const TableEntry &e)
        {
            auto w = e.window();
            int sum = 0;
            for (size_t i = 0; i < w.slot_size(); i++)
                sum += w.slot(i);
            return sum;
        }
    };

    std::unique_ptr<Server> server;
    thread *server_thread;

    void runServer(string addr, int click_instance_id)
    {
        curPos = 0;

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
        server->Shutdown();
        server_thread->join();
    }

    Table::Table()
    {
        tables.push_back(this);
    }

    string
    toString(const TableEntry &entry)
    {
        string ret("");
        ret += "TableName: " + entry.table_name();
        ret += "\tMatch: ";
        int size = entry.match_size();
        for (size_t i = 0; i < size; i++)
        {
            auto m = entry.match(i);
            ret += m.field_name() + ": " + m.exact().value() + "; ";
        }
        auto a = entry.action();
        size = a.params_size();
        ret += "\tAction: " + a.action() + " param: ";
        for (size_t i = 0; i < size; i++)
        {
            auto p = a.params(i);
            ret += p.param() + ": " + p.value() + "; ";
        }
        ret += "\tPriority: " + to_string(entry.priority());
        auto w = entry.window();
        size = w.slot_size();
        ret += "\tSlidingWindow(" + to_string(size) + "): ";
        for (size_t i = 0; i < size; i++)
            ret += to_string(w.slot(i)) + " ";
        return ret;
    }

    TableEntry *Table::lookup(const string &key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return NULL;
        auto e = &(it->second);
        incSlot(e);
        return e;
    }

    void incSlot(TableEntry *entry)
    {
        auto w = entry->mutable_window();
        w->set_slot(curPos, w->slot(curPos) + 1);
    }

    TableEntry *newTableEntry()
    {
        TableEntry *entry = new TableEntry();
        // init sliding window
        SlidingWindow *window = entry->mutable_window();
        for (int i = 0; i < WINDOW_SIZE; i++)
            window->add_slot(0);
        return entry;
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
            ret += m.exact().value();
        }
        return ret;
    }

} // namespace P4SFCState