g++ -o hash.o -c hash.cpp
g++ -o client client.cpp socketUtils.hpp hash.o -lcrypto -lssl