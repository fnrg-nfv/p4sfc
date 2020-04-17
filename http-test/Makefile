CXX=g++
CPPFLAGS=-std=c++11 -I../third_party/cpp-httplib

all: server client

client:
	$(CXX) $(CPPFLAGS) client.cc -c
	$(CXX) client.o -o client

server:
	$(CXX) $(CPPFLAGS) server.cc -c
	$(CXX) -pthread server.o -o server

clean:
	rm -rf *.o client server