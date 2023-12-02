
#include "peer.hpp"
int main() {
    Peer peer1 = Peer(2, 5, 65535, 65534, 65533, 0, 0, 0);
    vector<PeerInfo> peerList;
    PeerInfo pi;
    sprintf(pi.hostname, "%s", "vcm-32429.vm.duke.edu");
    pi.port = 65535;
    peerList.push_back(pi);
    peer1.joinP2P(peerList);
    int socket_fd = buildServer("65535");
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    while (true)
    {
        int ping_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        peer1.handlePing(ping_fd);
    }
    
    return 0;
}