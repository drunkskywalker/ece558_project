
struct PeerInfo_t
{
    char hostname[256];
    unsigned short int port; //listen for connection
};
typedef struct PeerInfo_t PeerInfo;


struct QueryId_t{
    char fileHash[256];
    char timeStamp[16];
    char initHost[256];
};
typedef struct QueryId_t QueryId;

struct Ping_t
{
    PeerInfo selfInfo;
};
typedef struct Ping_t Ping;

/**
 * With a list of peers
*/
struct Pong_t
{
    bool canConnect;
    int peerNum;
    PeerInfo peerList[10];
};
typedef struct Pong_t Pong;

struct Query_t
{
    QueryId id;
    char prevtHost[256];
    int TTL;
};
typedef struct Query_t Query;

struct QueryHit_t
{
    QueryId id;
    char prevtHost[256];
    PeerInfo destPeer;
};
typedef struct QueryHit_t QueryHit;


