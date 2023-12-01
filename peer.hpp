#include <map>
#include <vector>
#include "message.hpp"

using namespace std;
class Peer
{
    int initPeerNum;             // Initial peer number when setup
    int maxPeerNum;              // Maximum connection number
    unsigned short int pingPort; // Accept ping connection
    unsigned short int userPort; // Accept connection from user
    unsigned short int filePort; // Accept connection from peer for file
    int TTL;                     // TTL for query hop
    int timeToErase;             // Time for a query to be erased from table
    int timeToCheck;             // Time to check whether to erase

    map<string, PeerStore> peerMap;          // Host -> peer_info, socket_fd
    map<string, Query> queryForwardMap;      // QueryId ->  (prev host, time)
    map<string, string> filePathMap;         // QueryId -> file path
    map<string, QueryStatus> queryStatusMap; // File hash -> (find hit, time)

public:
    Peer(int ipn, int mpn,
         unsigned short int pprt, unsigned short int uprt, unsigned short int fprt,
         int ttl, int tte, int ttc) : initPeerNum(ipn), maxPeerNum(mpn),
                                      pingPort(pprt), userPort(uprt), filePort(fprt),
                                      TTL(ttl), timeToErase(tte), timeToCheck(ttc) {}

    /**
     * Join the peer-to-peer network, make connections to certain number of peers.
     * If it is in the famous address, just open ping port and wait for others to 
     * connect. If not, connect to facouse addresses and send ping to get a list
     * of potential peers. 
     * init ping and handle pong, BFS
     * @param famousIdList A list of famous address for this network
    */
    void joinP2P(vector<PeerInfo>& famousIdList);

    /**
     * After pingPort accept a connection, receive the ping struct and handle the
     * ping request. Send back a list of neighbors to that socket. Determine 
     * whether to add this peer to the neighbor list.
     * @param socket_fd The ping request socket
     * @recv Ping
     * @send Pong
    */
    void handlePing(int socket_fd);

    /**
     * Send the query to all neighbors, without sending to the prev and init host 
     * in the qry.
    */
    void sendAll(Query qry);

    void initQuery(string fileHash);

    void handleQuery(Query qry);

    void handleQueryHit(Query qry, string filePath);

    void initFileRequest(QueryId qid, PeerInfo pif);

    void handleFileRequest(QueryId qid, int socket_fd);
};