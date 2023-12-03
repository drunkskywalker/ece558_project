#include "peer.hpp"

#include <deque>
#include <set>

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
    if (selfInfo.hostname != pio.hostname) {
      peerQueue.push_back(pio);
    }
  }

  Ping selfPing;
  Pong currPong;
  selfPing.selfInfo = selfInfo;
  set<string> visited;
  // int status;
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
          currPong.peerList[i].hostname != selfInfo.hostname) {
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
  string key = genQueryIdString(qry.id);
  for (map<string, PeerStore>::iterator it = peerMap.begin(); it != peerMap.end(); ++it) {
    // self hostname is not prevHost, and
    // self hostname is not initHost, and
    // queryForwardMap does not have the query
    if (strcmp(selfInfo.hostname, qry.prevHost) != 0 &&
        strcmp(selfInfo.hostname, qry.id.initHost) != 0 &&
        queryForwardMap.find(key) == queryForwardMap.end()) {
      int target_fd = it->second.socket_fd;

      Query newQry = qry;
      sprintf(newQry.prevHost, "%s", selfInfo.hostname);

      // send type of Query first

      if (send(target_fd, TYPE_QUERY, sizeof(int), 0) == -1) {
        cout << "Failed to send to peer " << it->first << "\n";
        close(target_fd);
        peerMap.erase(it);
      }

      if (send(target_fd, &newQry, sizeof(Query), 0) < 0) {
        cout << "Failed to send to peer " << it->first << "\n";
        close(target_fd);
        peerMap.erase(it);
      }

      else {
        queryForwardMap[key] = qry;
      }
    }
  }
}