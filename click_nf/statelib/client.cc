#include <iostream>
#include <memory>
#include <string>
#include <iomanip>
#include <chrono>
#include <ctime>

#include <grpc++/grpc++.h>

#include "p4sfcstate.grpc.pb.h"
#include "p4sfcstate.hh"

using namespace grpc;
using namespace P4SFCState;

class RPCClient
{
public:
  RPCClient(std::shared_ptr<Channel> channel)
      : stub_(RPC::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string &user)
  {
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
    if (status.ok())
    {
      return reply.message();
    }
    else
    {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  void GetState(TableEntryReply *reply)
  {
    Empty request;
    ClientContext context;
    Status status = stub_->GetState(&context, request, reply);
    if (!status.ok())
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  }

private:
  std::unique_ptr<RPC::Stub> stub_;
};

int main(int argc, char **argv)
{
  RPCClient client(grpc::CreateChannel(
      "localhost:28282", grpc::InsecureChannelCredentials()));

  std::string user("world");
  std::string reply = client.SayHello(user);
  std::cout << "Greeter received: " << reply << std::endl;

  TableEntryReply teReply;
  auto t_start = std::chrono::high_resolution_clock::now();
  client.GetState(&teReply);
  auto t_end = std::chrono::high_resolution_clock::now();
  std::cout << std::fixed << std::setprecision(2)
            << "Get state time passed:"
            << std::chrono::duration<double, std::milli>(t_end - t_start).count() << " ms\n";
  std::cout << teReply.click_instance_id() << std::endl;
  int size = teReply.entries_size();
  if (size <= 100)
  {
    for (size_t i = 0; i < size; i++)
    {
      auto e = teReply.entries(i);
      std::cout << toString(e) << std::endl;
    }
  }
  else
    std::cout << "Entries size: " << size << std::endl;

  return 0;
}
