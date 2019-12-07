# PI HOME need to be confirmed
PI_HOME = /home/mjt/p4nfv/PI

SRCS=app.cpp simple_router_mgr.cpp
OBJS=$(subst .cpp,.o,$(SRCS))
ARC=$(PI_HOME)/src/libpip4info.la $(PI_HOME)/proto/libpiprotogrpc.la $(PI_HOME)/proto/libpiprotobuf.la $(PI_HOME)/proto/p4info/libpiconvertproto.la
DIST=controller

CC = gcc  # useless
CXX = g++
RM = rm -f

CXXFLAGS += -Wall -Werror -Wno-unused-command-line-argument -g -O2 -std=c++11
CPPFLAGS += -I$(PI_HOME)/proto -I$(PI_HOME)/include -I$(PI_HOME)/proto/frontend -I$(PI_HOME)/proto/server -I$(PI_HOME)/proto/cpp_out -I$(PI_HOME)/proto/grpc_out -I$(PI_HOME)/proto/p4info
LDFLAGS += -Wall -Werror -Wno-unused-command-line-argument -g -O2 -std=c++11
LDLIBS += -lmicrohttpd -lboost_system -lprotobuf -pthread -lpthread -L/usr/local/lib -lgrpc++ -lgrpc

LIBTOOL = $(PI_HOME)/proto/libtool --tag=CXX --mode=link  # TODO: this is not a perfect design

all: $(DIST)

$(DIST): $(OBJS)
	$(LIBTOOL) $(CXX) $(LDFLAGS) -o $(DIST) $(OBJS) $(ARC) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJS)
	$(RM) $(DIST)
