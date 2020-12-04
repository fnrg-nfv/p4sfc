#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include <grpc++/grpc++.h>

#include "p4sfcstate.grpc.pb.h"

using namespace grpc;
using namespace P4SFCState;

// Logic and data behind the server's behavior.
class ServiceImpl final : public RPC::Service {

  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status GetState(ServerContext* context, const Empty* request, TableEntryReply* response) {
    TableEntry* entry = response->add_entries();

    // entry->CopyFrom()


    return Status::OK;
  }

};

void RunServer() {
  std::string server_address("0.0.0.0:28282");
  ServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  std::thread th(RunServer);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
  return 0;
}
