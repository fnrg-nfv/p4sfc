#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "p4sfcstate.grpc.pb.h"
#include "p4sfcstate.hh"

using namespace grpc;
using namespace P4SFCState;

class RPCClient {
 public:
  RPCClient(std::shared_ptr<Channel> channel)
      : stub_(RPC::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  void GetState(TableEntryReply* reply) {
    Empty request;
    ClientContext context;
    Status status = stub_->GetState(&context, request, reply);
    if (!status.ok())
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  }

 private:
  std::unique_ptr<RPC::Stub> stub_;
};

int main(int argc, char** argv) {
  RPCClient client(grpc::CreateChannel(
      "localhost:28282", grpc::InsecureChannelCredentials()));

  std::string user("world");
  std::string reply = client.SayHello(user);
  std::cout << "Greeter received: " << reply << std::endl;

  TableEntryReply teReply;
  client.GetState(&teReply);
  int size = teReply.entries_size();
  for (size_t i = 0; i < size; i++) {
    auto e = teReply.entries(i);
    std::cout << toString(&e) << std::endl;
  }

  return 0;
}
