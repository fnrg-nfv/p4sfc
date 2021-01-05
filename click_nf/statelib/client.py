# -*- coding: utf-8 -*- 
import grpc
import p4sfcstate_pb2
import p4sfcstate_pb2_grpc
import time

channel = grpc.insecure_channel("10.176.35.14:28282")
client = p4sfcstate_pb2_grpc.RPCStub(channel)

for i in range(100):
    start = time.time()
    entry_request=p4sfcstate_pb2.Empty()
    entry_response=client.GetState(entry_request)
    end = time.time()
    print("GetState: %.3f s" % (end - start))