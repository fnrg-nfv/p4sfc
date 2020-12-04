#include <thread>
#include <grpc++/grpc++.h>

#include "p4sfcstate.hh"

#include "p4sfcstate.grpc.pb.h"


namespace P4SFCState {

using namespace grpc;

void incSlot(TableEntry *); 
string buildKey(TableEntry *);
int getSum(TableEntry *);

vector<Table*> tables;
int curPos;
bool isInit = false;

// Logic and data behind the server's behavior.
class ServiceImpl final : public RPC::Service {

    Status SayHello(ServerContext* context, const HelloRequest* request,
                    HelloReply* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return Status::OK;
    }

    Status GetState(ServerContext* context, const Empty* request, TableEntryReply* reply) {
        for(auto i = tables.begin(); i != tables.cend(); i++) {
            Table* table = *i;
            for (auto j = table->_map.begin(); j != table->_map.cend(); j++)
            {
                TableEntry* entry = reply->add_entries();
                entry->CopyFrom(j->second);
            }
        }
        return Status::OK;
    }

};

void runServer(string addr) {
    ServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << addr << std::endl;

    isInit = true;
    server->Wait();
}

thread* server;

void init(string addr) {
    curPos = 0;
    
    if (isInit)
        return;
    

    server = new thread(runServer, addr);
}

void end() {
    server->join();
}

TableEntry* newTableEntry() {
    TableEntry *entry = new TableEntry();
    // init sliding window
    SlidingWindow* window = entry->mutable_window();
    for (int i = 0; i < WINDOW_SIZE; i++)
        window->add_slot(0);
    return entry;
}

int Table::size() {
    return _map.size();
}

Table::Table() {
    tables.push_back(this);
}

int 
getSum(TableEntry *e){
    auto w = e->window();
    int size = w.slot_size();
    int sum = 0;
    for (int i = 0; i < size; i++)
        sum += w.slot(i);
    return sum;
}

// concat all val as key, seperated by commas
// entry maybe a true tableentry or something built from packet header
string 
buildKey(TableEntry *entry) {
    string ret("");
    int match_size = entry->match_size();
    for (int i = 0; i < match_size; i++) {
        auto m = entry->match(i);
        // TODO: only support exact currently
        ret += m.exact().value() + ",";
    }
    return ret;
}

void 
Table::insert(TableEntry *entry) {
    string key = buildKey(entry);
    _map.insert({key, *entry});
}

TableEntry*
Table::lookup(TableEntry *lookup_entry) {
    string key = buildKey(lookup_entry);
    auto it = _map.find(key);
    if (it == _map.end()) return NULL;
    auto e = &(it->second);
    incSlot(e);
    return e;
}

void 
incSlot(TableEntry *entry) {
    auto w = entry->mutable_window();
    w->set_slot(curPos, w->slot(curPos) + 1);
}

string 
toString(TableEntry *entry) {
    string ret("");
    ret += "TableName: " + entry->table_name();
    ret += "\tMatch: ";
    int size = entry->match_size();
    for (size_t i = 0; i < size; i++)
    {
        auto m = entry->match(i);
        ret += m.field_name() + ": " + m.exact().value() + "; ";
    }
    auto a = entry->action();
    size = a.params_size();
    ret += "\tAction: " + a.action() + " param: ";
    for (size_t i = 0; i < size; i++)
    {
        auto p = a.params(i);
        ret += p.param() + ": " + p.value() + "; ";
    }
    ret += "\tPriority: " + entry->priority();
    auto w = entry->window();
    size = w.slot_size();
    ret += "\tSlidingWindow(" + to_string(size) + "): ";
    for (size_t i = 0; i < size; i++)
        ret += to_string(w.slot(i)) + " "; 
    return ret;
}

void 
Table::remove(TableEntry *entry) {
    _map.erase(buildKey(entry));
}

}