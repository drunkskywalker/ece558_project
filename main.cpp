#include "peer.hpp"
int main() {
  Peer peer1 = Peer(2, 5, 65535, 65534, 65533, 5, 0, 0, "../example/folder2");
  vector<PeerInfo> peerList;
  PeerInfo pi;
  sprintf(pi.hostname, "%s", "vcm-32429.vm.duke.edu");
  pi.port = 65535;
  peerList.push_back(pi);
  peer1.run(peerList);
  return 0;
}