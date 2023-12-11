BIN=./bin
CPPFLAGS=-std=c++17 -pedantic -ggdb3 -Wall 
SSLFLAG=-lcrypto -lssl
# SRCS = $(wildcard *.cpp)
SRCS_peer=socketUtils.cpp peer.cpp hash.cpp main.cpp
OBJS_peer=$(patsubst %.cpp, $(BIN)/%.o, $(SRCS_peer))
SRCS_client=socketUtils.cpp hash.cpp client.cpp
OBJS_client=$(patsubst %.cpp, $(BIN)/%.o, $(SRCS_client))

PROGRAM= $(BIN)/client $(BIN)/main

all:  $(BIN)/client $(BIN)/main

$(BIN)/main: $(OBJS_peer)
	g++ $(CPPFLAGS) -o $@ $(OBJS_peer) $(SSLFLAG) -l pthread
# $(BIN)/testPingPong: $(OBJS_peer)
# 	g++ $(CPPFLAGS) -o $@ $(OBJS_peer) $(SSLFLAG) -l pthread
# $(BIN)/testQuery: $(OBJS_peer)
# 	g++ $(CPPFLAGS) -o $@ $(OBJS_peer) $(SSLFLAG) -l pthread
$(BIN)/client: $(OBJS_client)
	g++ $(CPPFLAGS) -o $@ $(OBJS_client) $(SSLFLAG)

$(BIN)/%.o: %.cpp socketUtils.hpp message.hpp peer.hpp hash.hpp 
	@mkdir -p $(BIN)
	g++ -c $(CPPFLAGS) $< -o $@

clean:
	rm -f $(OBJS_peer) $(OBJS_client) $(PROGRAM) *~
