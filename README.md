## Requirements
sudo apt-get install nlohmann-json3-dev

sudo apt-get install -y libssl-dev openssl

## Files
binaries are stored in ./bin folder.
./bin/main is the binary to run a p2p node, ./bin/client is the user interface.
## To Run
1. modify famous nodes in config.json, it should be ip address of a machine that is running. Feel free to change other parts of the config, ex. if you want to use a specific port
2. Run <code> make</code >
3. Run <code> ./bin/main</code > on all nodes
4. once the sockets are connected, Run <code>./bin/client</code> and follow the instruction. For example, you can enter Q for query, and then input the hash for your desired file
