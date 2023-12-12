sudo apt-get install nlohmann-json3-dev

sudo apt-get install -y libssl-dev openssl

binaries are stored in ./bin folder.

./bin/main is the binary to run a p2p node, ./bin/client is the user interface.

openssl 3.0 or above is required to build the application's secure build. main branch does not need this.

If you are using ubuntu22.04, you can install by running the command above.

If you are using ubuntu20.04, please follow the steps in this website to install openssl 3.0 https://orcacore.com/install-openssl-3-ubuntu-20-04/