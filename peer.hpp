#include <map>
#include <string>
#include <vector>

#include "message.hpp"

using namespace std;
class Peer {
  int initPeerNum;              // Initial peer number when setup
  int maxPeerNum;               // Maximum connection number
  unsigned short int pingPort;  // Accept ping connection
  unsigned short int userPort;  // Accept connection from user
  unsigned short int filePort;  // Accept connection from peer for file
  int TTL;                      // TTL for query hop
  int timeToErase;              // Time for a query to be erased from table
  int timeToCheck;              // Time to check whether to erase

  map<string, PeerStore> peerMap;           // Host -> peer_info, socket_fd
  map<string, Query> queryForwardMap;       // QueryId ->  (prev host, time)
  map<string, string> filePathMap;          // QueryId -> file path
  map<string, QueryStatus> queryStatusMap;  // File hash -> (find hit, time)

  /**
     * Join the peer-to-peer network, make connections to certain number of peers.
     * If it is in the famous address, just open ping port and wait for others to 
     * connect. If not, connect to facouse addresses and send ping to get a list
     * of potential peers. 
     * init ping and handle pong, BFS
     * @param famousIdList A list of famous address for this network
    */
  void joinP2P(vector<PeerInfo> & famousIdList);

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

  /**
     * Generate the Query with fileHash, time stamp and hostname
    */
  void initQuery(string fileHash);

  /**
     * Receive the Query, decide whether to sendAll or initQueryHit
    */
  void handleQuery(Query qry);

  /**
     * After finding the file in local directory, generate the Queryhit
     * with filePath and query id in the query
    */
  void initQueryHit(Query qry, string filePath);

  /**
     * After receiving a Queryhit and current hostname is not the initial
     * hostname in QueryHit's Query id, forward it according to the prev
     * address in queryForwardMap.
    */
  void forwardQueryHit(QueryHit qryh);

  /**
     * If the peer is the inital host that initiate the query, handle the
     * QueryHit.
     * If the QueryStatus in the queryStatusMap indicates it is already 
     * received a QueryHit, directly return.
     * Otherwise, change the status in queryStatusMap, and initFileRequest.
    */
  void handleQueryHit(QueryHit qryh);

  /**
     * 1. Connect to the destPeer in QueryHit
     * 2. Send QueryId to the destPeer
     * 3. Receive metadata:
     *      if status is ok: go to step 4
     *      otherwise: change back the QueryStatus and wait for other QueryHit.
     * 4. Prepare buffer based on the metadata, and receive content
     * 5. Check content by hashing and comparing with the fileHash
     * 6. Save content and send ok status to destPeer to indicate success
     * 7. Tell user about the status
    */
  void initFileRequest(QueryId qid, PeerInfo pif);

  /**
     * After receiving connection from the filePort, prepare to send the file
     * 1. Receive the QueryId and find it in filePathMap 
     * 2. If the QueryId is valid and file exits, send true in meta data
     *      Otherwise, send false in meta data
     *    Put file name, buffer size in meta data
     * 3. Send content through the socket
     * 4. Receive status from the socket, close socket, and delete the field 
     *    from queryForwardMap
    */
  void handleFileRequest(int socket_fd);

  /**
     * A thread to wait for connection and process p2p connection
    */
  void runPingPort(unsigned short int port);

  /**
     * A thread to wait for connection from client process and receive
     * request. Call the initQuery
    */
  void runUserPort(unsigned short int port);

  /**
     * A thread to wait for connection from peers to send file
    */
  void runFilePort(unsigned short int port);

 public:
  Peer(int ipn,
       int mpn,
       unsigned short int pprt,
       unsigned short int uprt,
       unsigned short int fprt,
       int ttl,
       int tte,
       int ttc) :
      initPeerNum(ipn),
      maxPeerNum(mpn),
      pingPort(pprt),
      userPort(uprt),
      filePort(fprt),
      TTL(ttl),
      timeToErase(tte),
      timeToCheck(ttc) {}

  /**
     * Run the service
     * 1. joinP2P network and open a thread for receiving ping
     * 2. open file port and client port in different thread
    */
  void run();
};