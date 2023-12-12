#include "peer.hpp"

#include <sys/sendfile.h>
string Peer::genQueryIdString(QueryId id) {
  std::stringstream ss;
  ss << id.fileHash << id.timeStamp << id.initHost;
  return ss.str();
}

int Peer::joinP2P(vector<PeerInfo> & famousIdList) {
  memset(&selfInfo, 0, sizeof(selfInfo));
  if (gethostname(selfInfo.hostname, sizeof(selfInfo.hostname)) == -1) {
    cerr << "Can not get host name correctly!\n";
    return -1;
  }
  selfInfo.port = pingPort;
  deque<PeerInfo> peerQueue;

  for (PeerInfo pio : famousIdList) {
    if (strcmp(selfInfo.hostname, pio.hostname) != 0) {
      peerQueue.push_back(pio);
    }
  }

  Ping selfPing;
  Pong currPong;
  selfPing.selfInfo = selfInfo;
  set<string> visited;
  // int status;
  std::lock_guard<std::mutex> guard(peerLock);
  while (!peerQueue.empty() && peerMap.size() < initPeerNum) {
    PeerInfo currPeer = peerQueue.front();
    peerQueue.pop_front();
    if (visited.find(string(currPeer.hostname)) != visited.end()) {
      continue;
    }
    visited.insert(string(currPeer.hostname));
    int currSocket =
        request_connection(currPeer.hostname, to_string(currPeer.port).c_str());
    if (currSocket < 0)
      continue;
    if (send(currSocket, &selfPing, sizeof(selfPing), 0) < 0) {
      close(currSocket);
      continue;
    }
    memset(&currPong, 0, sizeof(currPong));
    if (recv(currSocket, &currPong, sizeof(currPong), MSG_WAITALL) < 0) {
      close(currSocket);
      continue;
    }
    if (currPong.canConnect) {
      PeerStore currStore;
      currStore.peerinfo = currPeer;
      currStore.socket_fd = currSocket;
      peerMap[string(string(currPeer.hostname))] = currStore;
    }
    for (int i = 0; i < currPong.peerNum; i++) {
      if (visited.find(string(currPong.peerList[i].hostname)) == visited.end() &&
          strcmp(currPong.peerList[i].hostname, selfInfo.hostname) != 0) {
        peerQueue.push_back(currPong.peerList[i]);
      }
    }
  }
  cout << "Current Host Name is: " << selfInfo.hostname << ", port is " << selfInfo.port
       << endl;
  cout << "Connected peers number: " << peerMap.size() << endl;
  return 0;
}

void Peer::handlePing(int socket_fd) {
  Ping currPing;
  memset(&currPing, 0, sizeof(currPing));
  if (recv(socket_fd, &currPing, sizeof(currPing), MSG_WAITALL) < 0) {
    close(socket_fd);
    return;
  }
  Pong resultPong;
  PeerStore resultStore;
  resultStore.peerinfo = currPing.selfInfo;
  resultStore.socket_fd = socket_fd;
  std::lock_guard<std::mutex> guard(peerLock);
  if (peerMap.find(string(currPing.selfInfo.hostname)) != peerMap.end()) {
    resultPong.canConnect = true;
    peerMap[string(currPing.selfInfo.hostname)] = resultStore;
  }
  else {
    if (peerMap.size() < maxPeerNum) {
      resultPong.canConnect = true;
      peerMap[string(currPing.selfInfo.hostname)] = resultStore;
    }
    else {
      resultPong.canConnect = false;
    }
  }
  resultPong.peerNum = 0;
  for (map<string, PeerStore>::iterator it = peerMap.begin(); it != peerMap.end(); ++it) {
    if (it->first != currPing.selfInfo.hostname) {
      resultPong.peerList[resultPong.peerNum] = it->second.peerinfo;
      resultPong.peerNum++;
    }
  }
  send(socket_fd, &resultPong, sizeof(resultPong), 0);
  if (!resultPong.canConnect) {
    close(socket_fd);
  }
  else {
    cout << "Connected with " << currPing.selfInfo.hostname << endl;
  }
}

void Peer::sendAll(Query qry) {
  // if ttl is 0, the query has reached max hop. ignore
  qry.TTL--;
  if (qry.TTL <= 0) {
    return;
  }
  std::lock_guard<std::mutex> guard(peerLock);
  for (map<string, PeerStore>::iterator it = peerMap.begin(); it != peerMap.end(); ++it) {
    // target hostname is not prevHost, and target hostname is not initHost
    if ((strcmp(it->first.c_str(), qry.prevHost) != 0 &&
         strcmp(it->first.c_str(), qry.id.initHost) != 0)) {
      int target_fd = it->second.socket_fd;

      Query newQry = qry;
      sprintf(newQry.prevHost, "%s", selfInfo.hostname);

      // send type of Query first
      int flag = TYPE_QUERY;
      if (send(target_fd, &flag, sizeof(int), 0) < 0) {
        cout << "Failed to send to peer " << it->first << "\n";
        close(target_fd);
        peerMap.erase(it);
      }

      if (send(target_fd, &newQry, sizeof(Query), 0) < 0) {
        cout << "Failed to send to peer " << it->first << "\n";
        close(target_fd);
        peerMap.erase(it);
      }
      cout << "Foward request to " << it->first << endl;
    }
  }
}

void Peer::initQuery(string fileHash) {
  Query qry;
  memset(&qry, 0, sizeof(qry));
  qry.id.timeStamp = time(NULL);
  sprintf(qry.id.fileHash, "%s", fileHash.c_str());
  sprintf(qry.id.initHost, "%s", selfInfo.hostname);
  qry.TTL = TTL + 1;
  sprintf(qry.prevHost, "%s", selfInfo.hostname);

  // query status map: saves if the query is successfully responded
  QueryStatus qs;
  qs.finished = false;
  qs.timeStamp = qry.id.timeStamp;
  std::lock_guard<std::mutex> queryGuard(queryStatusLock);
  queryStatusMap[fileHash] = qs;

  string queryId = genQueryIdString(qry.id);

  // query forward map: this query has been forwarded. (in this case sent by self)

  sendAll(qry);
  std::lock_guard<std::mutex> guard(queryForwardLock);
  queryForwardMap[queryId] = qry;
}

void Peer::handleQuery(Query qry) {
  string queryId = genQueryIdString(qry.id);
  std::lock_guard<std::mutex> guard(queryForwardLock);
  // only process unprocessed query
  if (queryForwardMap.find(queryId) == queryForwardMap.end()) {
    queryForwardMap[queryId] = qry;
    string fileHash = qry.id.fileHash;
    // self has the file
    if (checkFileExist(fileHash, fileDir)) {
      string filePath = findFileName(fileHash, fileDir);
      {
        std::lock_guard<std::mutex> guard(filePathLock);
        filePathMap[queryId] = filePath;
      }
      initQueryHit(qry);
    }
    // self doesn't have the file, forward to others
    else {
      sendAll(qry);
    }
  }
}

void Peer::sendQueryHit(QueryHit qryh, string prevHost, int target_fd) {
  int flag = TYPE_QUERYHIT;
  if (send(target_fd, &flag, sizeof(int), 0) < 0) {
    cout << "Failed to send QueryHit to peer " << prevHost << "\n";
    close(target_fd);
    peerMap.erase(prevHost);
    return;
  }
  if (send(target_fd, &qryh, sizeof(qryh), 0) < 0) {
    cout << "Failed to send QueryHit to peer " << prevHost << "\n";
    close(target_fd);
    peerMap.erase(prevHost);
  }
}

void Peer::initQueryHit(Query qry) {
  QueryHit newQryH;
  newQryH.id = qry.id;
  newQryH.destPeer = selfInfo;
  newQryH.destPeer.port = filePort;
  string prevHost = string(qry.prevHost);
  // std::lock_guard<std::mutex> guard(peerLock);
  int target_fd = peerMap[prevHost].socket_fd;
  cout << "Init queryHit and send back to " << prevHost << endl;
  sendQueryHit(newQryH, prevHost, target_fd);
}

void Peer::forwardQueryHit(QueryHit qryh) {
  string key = genQueryIdString(qryh.id);
  string targetHost = "";
  int target_fd = -1;
  std::lock_guard<std::mutex> guard(queryForwardLock);
  if (queryForwardMap.find(key) != queryForwardMap.end()) {
    targetHost = string(queryForwardMap[key].prevHost);
  }
  if (peerMap.find(targetHost) != peerMap.end()) {
    target_fd = peerMap[targetHost].socket_fd;
  }
  if (targetHost.size() < 1 || target_fd < 0) {
    cout << "Failed to find previous host socket in the peerMap: " << targetHost << " "
         << target_fd << endl;
    return;
  }
  sendQueryHit(qryh, targetHost, target_fd);
}

void Peer::handleQueryHit(QueryHit qryh) {
  // If is initial host -> start file request
  if (strcmp(qryh.id.initHost, selfInfo.hostname) == 0) {
    string key(qryh.id.fileHash);
    std::lock_guard<std::mutex> guard(queryStatusLock);
    if (queryStatusMap.find(key) != queryStatusMap.end() &&
        !queryStatusMap[key].finished) {
      queryStatusMap[key].finished = true;
      // If failed: change finish in the initFileRequest method
      initFileRequest(qryh.id, qryh.destPeer);
      return;
    }
  }
  else {
    forwardQueryHit(qryh);
  }
}

void Peer::initFileRequest(QueryId qid, PeerInfo pif) {
  string key(qid.fileHash);
  int dest_fd = request_connection(pif.hostname, to_string(pif.port).c_str());
  if (dest_fd < 0) {
    std::lock_guard<std::mutex> guard(queryStatusLock);
    queryStatusMap[key].finished = false;
    return;
  }
  if (send(dest_fd, &qid, sizeof(qid), 0) < 0) {
    close(dest_fd);
    std::lock_guard<std::mutex> guard(queryStatusLock);
    queryStatusMap[key].finished = false;
    return;
  }
  ContentMeta fileMeta;
  memset(&fileMeta, 0, sizeof(fileMeta));
  if (recv(dest_fd, &fileMeta, sizeof(fileMeta), MSG_WAITALL) < 0) {
    close(dest_fd);
    std::lock_guard<std::mutex> guard(queryStatusLock);
    queryStatusMap[key].finished = false;
    return;
  }
  if (!fileMeta.status) {
    close(dest_fd);
    std::lock_guard<std::mutex> guard(queryStatusLock);
    queryStatusMap[key].finished = false;
    return;
  }
  vector<char> content;
  char buffer[2048];
  size_t currLen = 0;
  int len;
  while (currLen < fileMeta.length) {
    memset(&buffer, 0, sizeof(buffer));
    if ((len = recv(dest_fd, &buffer, sizeof(buffer), MSG_WAITALL)) < 0) {
      close(dest_fd);
      std::lock_guard<std::mutex> guard(queryStatusLock);
      queryStatusMap[key].finished = false;
      return;
    }
    currLen += len;
    content.insert(content.end(), buffer, buffer + len);
  }
  string currHash = getVectorCharHash(content);
  if (strcmp(currHash.c_str(), qid.fileHash) != 0) {
    close(dest_fd);
    std::lock_guard<std::mutex> guard(queryStatusLock);
    queryStatusMap[key].finished = false;
    return;
  }
  ofstream saveFile;
  saveFile.open(fileDir + fileMeta.fileName);
  saveFile << string(content.begin(), content.end());
  saveFile.close();
}

void Peer::handleFileRequest(int socket_fd) {
  QueryId qid;
  memset(&qid, 0, sizeof(qid));
  if (recv(socket_fd, &qid, sizeof(qid), MSG_WAITALL) < 0) {
    close(socket_fd);
    return;
  }
  cout << "Peer " << qid.initHost << " ask for " << qid.fileHash << endl;
  ContentMeta resMeta;
  memset(&resMeta, 0, sizeof(resMeta));
  string key = genQueryIdString(qid);
  if (filePathMap.find(key) != filePathMap.end()) {
    string filePath = filePathMap[key];

    FILE * fptr = fopen(filePath.c_str(), "rb");
    if (fptr == NULL) {
      cout << "Failed to open file " << filePath << endl;
      resMeta.status = false;
      filePathMap.erase(key);
      send(socket_fd, &resMeta, sizeof(resMeta), 0);
      close(socket_fd);
      return;
    }
    // get file size
    fseek(fptr, 0, SEEK_END);
    int fileSize = ftell(fptr);
    resMeta.status = true;
    resMeta.length = fileSize;
    string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
    sprintf(resMeta.fileName, "%s", fileName.c_str());
    off_t offset = 0;
    int bytesSent = 1;
    if (send(socket_fd, &resMeta, sizeof(resMeta), 0) > 0) {
      cout << "Send metadata to " << qid.initHost << " with content lenght "
           << resMeta.length << endl;
      if (fptr) {
        while (bytesSent > 0) {
          bytesSent = sendfile(socket_fd, fptr->_fileno, &offset, 2048);
          std::cout << "sended bytes : " << offset << '\n';
        }
      }
      if (offset == fileSize) {
        cout << "Send all content to " << qid.initHost << endl;
        filePathMap.erase(key);
      }
      // if (send(socket_fd, content_cstr, strlen(content_cstr), 0) >= 0) {
      //   cout << "Send content to " << qid.initHost << endl;
      //   filePathMap.erase(key);
      // }
      // }
      fclose(fptr);
      close(socket_fd);
      return;
    }
  }
  // } else {
  //     resMeta.status = false;
  //     filePathMap.erase(key);
  //     send(socket_fd, &resMeta, sizeof(resMeta), 0);
  //     close(socket_fd);
  //     return;
  // }
}

void Peer::runSelect() {
  fd_set peersFDSet;

  // set timeout
  struct timeval time;
  time.tv_sec = 60;
  time.tv_usec = 0;

  while (true) {
    FD_ZERO(&peersFDSet);
    int nfds = 0;
    std::lock_guard<std::mutex> guard(peerLock);
    for (map<string, PeerStore>::iterator it = peerMap.begin(); it != peerMap.end();
         ++it) {
      FD_SET(it->second.socket_fd, &peersFDSet);
      if (it->second.socket_fd > nfds) {
        nfds = it->second.socket_fd;
      }
    }
    nfds++;
    int status = select(nfds, &peersFDSet, NULL, NULL, &time);
    errorHandle(status, "Error: select error", NULL, NULL);
    if (status == 0) {
      // cout << "listen time limit" << endl;
      continue;
    }
    else {
      for (map<string, PeerStore>::iterator it = peerMap.begin(); it != peerMap.end();
           ++it) {
        int target_fd = it->second.socket_fd;
        if (FD_ISSET(target_fd, &peersFDSet)) {  // find socket that received data
          // recieve type
          int queryType = 0;
          status = recv(target_fd, &queryType, sizeof(int), 0);
          errorHandle(status, "Error: select error", NULL, NULL);
          // recieve query or query hit
          if (queryType == TYPE_QUERY) {
            Query qry;
            status = recv(target_fd, &qry, sizeof(Query), 0);
            errorHandle(status, "Error: Receive query error", NULL, NULL);
            cout << "received query from " << qry.prevHost << endl;

            handleQuery(qry);
          }
          else if (queryType == TYPE_QUERYHIT) {
            QueryHit qryh;
            status = recv(target_fd, &qryh, sizeof(QueryHit), 0);
            errorHandle(status, "Error: Receive queryHit error", NULL, NULL);
            handleQueryHit(qryh);
          }
        }
      }
    }
  }
}

void Peer::runPingPort(unsigned short int port) {
  int ping_fd = buildServer(to_string(port).c_str());
  while (true) {
    int curr_fd = try_accept(ping_fd);
    if (curr_fd != -1) {
      handlePing(curr_fd);
    }
  }
}

void Peer::runUserPort(unsigned short int port) {
  int user_fd = buildServer(to_string(port).c_str());
  char recvHash[128];
  while (true) {
    int curr_fd = try_accept(user_fd);
    if (curr_fd != -1) {
      memset(&recvHash, 0, sizeof(recvHash));
      if (recv(curr_fd, &recvHash, 64, MSG_WAITALL) > 0) {
        string hash_str(recvHash);
        std::lock_guard<std::mutex> guard(queryStatusLock);
        if (hash_str.length() == 64 &&
            queryStatusMap.find(hash_str) == queryStatusMap.end()) {
          initQuery(hash_str);
        }
      }
      close(curr_fd);
    }
  }
}

void Peer::runFilePort(unsigned short int port) {
  int file_fd = buildServer(to_string(port).c_str());
  while (true) {
    int curr_fd = try_accept(file_fd);
    if (curr_fd != -1) {
      cout << "Had a connection on file port" << endl;
      handleFileRequest(curr_fd);
    }
  }
}

void Peer::runCheckTimeout1() {
  std::lock_guard<std::mutex> guard(queryStatusLock);
  map<string, QueryStatus>::iterator it = queryStatusMap.begin();
  while (it != queryStatusMap.end()) {
    if (it->second.finished) {
      it = queryStatusMap.erase(it);
    }
    else if (time(NULL) - it->second.timeStamp > timeToErase) {
      it = queryStatusMap.erase(it);
    }
    else {
      ++it;
    }
  }
}

void Peer::checkLoop1() {
  while (true) {
    sleep(timeToCheck);
    runCheckTimeout1();
  }
}
void Peer::runCheckTimeout2() {
  std::lock_guard<std::mutex> guard(queryForwardLock);
  map<string, Query>::iterator it2 = queryForwardMap.begin();
  while (it2 != queryForwardMap.end()) {
    if (time(NULL) - it2->second.id.timeStamp > timeToErase) {
      it2 = queryForwardMap.erase(it2);
    }
    else {
      ++it2;
    }
  }
}

void Peer::checkLoop2() {
  while (true) {
    sleep(timeToCheck);
    runCheckTimeout2();
  }
}

void Peer::run(vector<PeerInfo> & famousIdList) {
  if (joinP2P(famousIdList) < 0) {
    exit(EXIT_FAILURE);
  }

  thread ping_t(&Peer::runPingPort, this, pingPort);
  // thread user_t(&runUserPort,this,userPort);
  thread file_t(&Peer::runFilePort, this, filePort);
  thread select_t(&Peer::runSelect, this);
  thread check_t(&Peer::checkLoop1, this);
  thread check_t2(&Peer::checkLoop1, this);

  ping_t.detach();
  // user_t.detach();
  file_t.detach();
  select_t.detach();
  check_t.detach();
  check_t2.detach();

  runUserPort(userPort);
}
