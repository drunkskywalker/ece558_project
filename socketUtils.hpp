#ifndef SOCKETUTIL
#define SOCKETUTIL
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
using namespace std;
void errorHandle(int status, std::string message,const char * hostname, const char * port);
void connectionEnd(int byte);
int buildServer(const char * port);
int buildClient(const char * hostname, const char * port);
int request_connection(const char *hostname, const char *port );
int try_accept(int socket_fd);

#endif
