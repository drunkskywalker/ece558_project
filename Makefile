CPPFLAGS=-Wall  -std=c++11 -pedantic -ggdb3
SRCS= socketUtils.cpp peer.cpp testPingPong.cpp
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
PROGRAM=testPingPong

$(PROGRAM): $(OBJS)
	g++ $(CPPFLAGS) -o $@ $(OBJS)

%.o: %.cpp socketUtils.hpp message.hpp peer.hpp
	g++ -c $(CPPFLAGS) $<

clean:
	rm -f $(OBJS) $(PROGRAM) *~