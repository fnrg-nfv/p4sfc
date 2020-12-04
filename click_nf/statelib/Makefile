
PROTOS_PATH = .
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread -fPIC
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`


all: client server test libstate.so

libstate.so: p4sfcstate.pb.o p4sfcstate.grpc.pb.o p4sfcstate.o
	$(CXX) $^ $(LDFLAGS) -shared -o $@

test: test.o
	$(CXX) $^ $(LDFLAGS) -L. -lstate -o $@

client: p4sfcstate.pb.o p4sfcstate.grpc.pb.o p4sfcstate.o client.o
	$(CXX) $^ $(LDFLAGS) -o $@

server: p4sfcstate.pb.o p4sfcstate.grpc.pb.o server.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $^


# .PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

# .PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

proto: p4sfcstate.pb.cc p4sfcstate.grpc.pb.cc 

clean:
	rm -f *.o *.so *.pb.cc *.pb.h client server test
