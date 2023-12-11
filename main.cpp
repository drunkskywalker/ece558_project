#include <nlohmann/json.hpp>

#include "peer.hpp"
using json = nlohmann::json;

json readJson(string filename) {
    std::ifstream file(filename);

    // Check if the file is opened successfully
    if (!file.is_open()) {
        std::cerr << "====================\nError opening file\n====================\n";
        return 1;
    }

    // Parse the JSON data
    json jsonData;
    file >> jsonData;

    // Close the file
    file.close();
    return jsonData;
}
// int main() {
//     json jsonData = readJson("./config.json");

//     int initPeerNum = jsonData["initPeerNum"];
//     int maxPeerNum = jsonData["maxPeerNum"];
//     int pingPort = jsonData["pingPort"];
//     int userPort = jsonData["userPort"];
//     int filePort = jsonData["filePort"];
//     int TTL = jsonData["TTL"];
//     int timeToErase = jsonData["timeToErase"];
//     int timeToCheck = jsonData["timeToCheck"];
//     string dir = jsonData["directory"];
//     Peer peer1(initPeerNum, maxPeerNum, pingPort, userPort, filePort, TTL, timeToErase, timeToCheck, dir);
//     vector<PeerInfo> peerList;
//     for (const auto& node : jsonData["famousNodes"]) {
//         PeerInfo pi;
//         memset(&pi, 0, sizeof(pi));
//         strncpy(pi.hostname, "vcm-37283.vm.duke.edu", sizeof(pi.hostname));
//         pi.hostname[sizeof(pi.hostname) - 1] = '\0';
//         pi.port = 65535;
//         peerList.push_back(pi);
//     }

//     peer1.run(peerList);
//     return 0;
// }
int main() {
    Peer peer1 = Peer(2, 5, 65535, 65534, 65533, 5, 0, 0, "./example/folder4/");
    vector<PeerInfo> peerList;
    PeerInfo pi;
    sprintf(pi.hostname, "%s", "vcm-37254.vm.duke.edu");
    pi.port = 65535;
    peerList.push_back(pi);
    peer1.run(peerList);
    return 0;
}