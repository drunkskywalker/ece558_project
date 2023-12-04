
#include "socketUtils.hpp"
void errorHandle(int status,
                 std::string message,
                 const char * hostname,
                 const char * port) {
  if (status < 0) {
    std::cerr << message << std::endl;
    if (hostname != NULL && port != NULL) {
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    }
    // exit(EXIT_FAILURE);
  }
}
void connectionEnd(int byte) {
  // std::string message = "Connection ends";
  if (byte == 0) {
    std::cerr << "Connection ends" << std::endl;
    exit(EXIT_FAILURE);
  }
  else {
    errorHandle(byte, "Error: Recieve failed", NULL, NULL);
  }
}

int buildServer(const char * port) {
  const char * hostname = NULL;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  int status, socket_fd;

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  errorHandle(status, "Error: cannot get address info for host", hostname, port);

  //generate a port
  if (string(port) == "") {
    ((struct sockaddr_in *)host_info_list->ai_addr)->sin_port = 0;
  }

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  errorHandle(socket_fd, "Error: cannot create socket", hostname, port);

  int yes = 1;
  status =
      setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));  //reuse port
  errorHandle(status, "Error: setsocketopt", hostname, port);

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  errorHandle(status, "Error: cannot bind socket", hostname, port);

  status = listen(socket_fd, 100);
  errorHandle(status, "Error: cannot listen on socket", hostname, port);

  freeaddrinfo(host_info_list);
  return socket_fd;
}

int buildClient(const char * hostname, const char * port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  errorHandle(status, "Error: cannot get address info for host", hostname, port);

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  errorHandle(socket_fd, "Error: cannot create socket", hostname, port);

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  errorHandle(status, "Error: cannot connect to socket", hostname, port);
  freeaddrinfo(host_info_list);
  return socket_fd;
}

int request_connection(const char * hostname, const char * port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  //Get host infomation
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  //Setup socket
  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  //Build connection
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  freeaddrinfo(host_info_list);
  return socket_fd;
}

int try_accept(int socket_fd) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_fd == -1) {
      std::cerr << "Error on Accept: cannot accept a connection" << std::endl;
      return -1;
    }
    return client_fd;
  }