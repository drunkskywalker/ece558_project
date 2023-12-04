#include <sys/socket.h>

#define TYPE_QUERY 101
#define TYPE_QUERYHIT 102

struct PeerInfo_t {
  char hostname[256];
  unsigned short int port;  //listen for connection
};
typedef struct PeerInfo_t PeerInfo;

struct PeerStore_t {
  PeerInfo peerinfo;
  int socket_fd;
};
typedef struct PeerStore_t PeerStore;

struct QueryStatus_t {
  bool finished;
  size_t timeStamp;
};
typedef struct QueryStatus_t QueryStatus;

struct QueryId_t {
  char fileHash[128];
  size_t timeStamp;
  char initHost[256];
};
typedef struct QueryId_t QueryId;

struct Ping_t {
  PeerInfo selfInfo;
};
typedef struct Ping_t Ping;

/**
 * With a list of peers
*/
struct Pong_t {
  bool canConnect;
  int peerNum;
  PeerInfo peerList[10];
};
typedef struct Pong_t Pong;

struct Query_t {
  QueryId id;
  char prevHost[256];
  int TTL;
};
typedef struct Query_t Query;

struct QueryHit_t {
  QueryId id;
  // char prevtHost[256];
  PeerInfo destPeer;
};
typedef struct QueryHit_t QueryHit;

struct ContentMeta_t {
  bool status;
  char fileName[256];
  size_t length;
};
typedef struct ContentMeta_t ContentMeta;
