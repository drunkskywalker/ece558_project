
CC=g++
CFLAGS=-std=c++17 -Wall
LDFLAGS=-lcrypto -lssl
TARGET=hash
SOURCES=hash.cpp
OBJECTS=$(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
