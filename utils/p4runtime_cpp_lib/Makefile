LIB_SRCS=libswitch.cpp general_switch.cpp
LIB_OBJS=$(subst .cpp,.o,$(LIB_SRCS))
LIB_DIST=libswitch.so

CC = gcc
CXX = g++
RM = rm -f

# CXXFLAGS & LDFLAGS
FLAGS += -Wall -Werror -Wno-unused-command-line-argument -g -O2 -std=c++11
LDLIBS += -lmicrohttpd -lboost_system -lprotobuf -pthread -lpthread -L/usr/local/lib -lgrpc++ -lgrpc
LDLIBS += -lpip4info -lpiprotogrpc -lpiprotobuf -lpiconvertproto

all: $(LIB_DIST) app

$(LIB_DIST): $(LIB_OBJS)
	$(CXX) $(FLAGS) -shared -o $(LIB_DIST) $(LIB_OBJS) $(LDLIBS)

app:
	$(CC) -Wall -Werror -Wno-unused-command-line-argument -g -O2 -o app app.c -L. -lswitch -lpthread

%.o: %.cpp
	$(CXX) $(FLAGS) -fPIC -c -o $@ $<

clean: cleanapp
	$(RM) $(LIB_OBJS)
	$(RM) $(LIB_DIST)

cleanapp:
	$(RM) app