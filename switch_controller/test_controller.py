import grpc
import p4sfcstate_pb2
import p4sfcstate_pb2_grpc

if __name__ == '__main__':
    channel = grpc.insecure_channel("127.0.0.1:28282")
    client = p4sfcstate_pb2_grpc.RPCStub(channel)

    hello_request = p4sfcstate_pb2.HelloRequest()
    hello_request.name = "Jason"

    hello_response = client.SayHello(hello_request)
    print hello_response.message

    state_request = p4sfcstate_pb2.Empty()

    state_response = client.GetState(state_request)

    for entry in state_response.entries:
        print entry
        print "------------------"
