#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "p4sfcstate.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
// using P4SFCState::HelloRequest;
// using P4SFCState::HelloReply;
// using P4SFCState::RPC;
// using P4SFCState::Empty;
// using P4SFCState::TableEntryReply;
using namespace P4SFCState;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public RPC::Service {

  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status GetState(ServerContext* context, const Empty* request, TableEntryReply* response) {
    TableEntry* entry = response->add_entries();
    FieldMatch* match = entry->add_match();
  

    return Status::OK;
  }

};

void RunServer() {
  std::string server_address("0.0.0.0:28282");
  GreeterServiceImpl service;

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
  RunServer();

  return 0;
}
