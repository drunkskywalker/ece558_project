BIN=./bin
CPPFLAGS=-std=c++11 -pedantic -ggdb3 -Wall 
SSLFLAG=-lcrypto -lssl
# SRCS = $(wildcard *.cpp)
SRCS_peer=socketUtils.cpp peer.cpp testPingPong.cpp hash.cpp testQuery.cpp
OBJS_peer=$(patsubst %.cpp, $(BIN)/%.o, $(SRCS_peer))
SRCS_client=socketUtils.cpp hash.cpp client.cpp
OBJS_client=$(patsubst %.cpp, $(BIN)/%.o, $(SRCS_client))

PROGRAM=$(BIN)/testPingPong $(BIN)/client

all: $(BIN)/testPingPong $(BIN)/client

$(BIN)/testPingPong: $(OBJS_peer)
	g++ $(CPPFLAGS) -o $@ $(OBJS_peer) $(SSLFLAG) -l pthread
$(BIN)/testQuery: $(OBJS_peer)
	g++ $(CPPFLAGS) -o $@ $(OBJS_peer) $(SSLFLAG) -l pthread
$(BIN)/client: $(OBJS_client)
	g++ $(CPPFLAGS) -o $@ $(OBJS_client) $(SSLFLAG)

$(BIN)/%.o: %.cpp socketUtils.hpp message.hpp peer.hpp hash.hpp 
	@mkdir -p $(BIN)
	g++ -c $(CPPFLAGS) $< -o $@

clean:
	rm -f $(OBJS_peer) $(OBJS_client) $(PROGRAM) *~
