
#include "peer.hpp"
int main() {
    Peer peer1 = Peer(2, 5, 65535, 65534, 65533, 0, 0, 0);
    vector<PeerInfo> peerList;
    PeerInfo pi;
    sprintf(pi.hostname, "%s", "vcm-32429.vm.duke.edu");
    pi.port = 65535;
    peerList.push_back(pi);
    peer1.joinP2P(peerList);
    buildServer("65535");
    return 0;
}