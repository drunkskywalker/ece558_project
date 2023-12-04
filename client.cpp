#include <nlohmann/json.hpp>
#include <stdio.h>
#include <sys/stat.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "hash.hpp"
#include "socketUtils.hpp"

using json = nlohmann::json;
using namespace std;

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

int sendQuery(int socket, string & hash) {
  int status = send(socket, hash.c_str(), 64, 0);
  errorHandle(status, "Error: send hash failed", NULL, NULL);
  return status;
}

int uploadFile(string & path, string & directory) {
  string command = "cp " + path + " " + directory;
  int result = std::system(command.c_str());
  if (result == 0) {
    std::cout << "====================\nSuccessfully uploaded.\n====================\n";
  }
  else {
    std::cerr << "====================\nError executing command.\n====================\n";
  }
  return result;
}

void query(int & socket, string & dir) {
  cout << "Input the hash of the file you wish to query:\n";
  string hash;
  cin >> hash;

  if (checkValidSHA256(hash)) {
    if (checkFileExist(hash, dir)) {
      cout << "====================\nFile already exists.\n====================\n";
    }
    else {
      cout << "====================\nFile does not exist. sending "
              "query...\n====================\n";
      sendQuery(socket, hash);
    }
  }
  else {
    cout << "====================\nHash is not valid\n====================\n";
  }
}

void check(string & dir) {
  cout << "Input the hash of the file you wish to check:\n";
  string hash;
  cin >> hash;

  if (checkValidSHA256(hash)) {
    if (checkFileExist(hash, dir)) {
      cout << "====================\nFile exists.\n====================\n";
    }
    else {
      cout << "====================\nFile does not exist.\n====================\n";
    }
  }
  else {
    cout << "====================\nHash is not valid.\n====================\n";
  }
}

int directoryCheck(string & dir) {
  DIR * directory;
  if ((directory = opendir(dir.c_str())) == NULL) {
    cout << "====================\nDirectory " << dir
         << " does not exist. Creating directory...\n====================\n";
    if (mkdir(dir.c_str(), 0777) != 0) {
      cout << "====================\nError creating directory. Check if you are "
              "permitted to do "
              "so.\n====================\n";
      return 1;
    }
    else {
      cout << "====================\nSuccessfully created "
              "directory.\n====================\n";
    }
  }
  return 0;
}

void help() {
  cout << "What do you want to do?\n====================\n"
       << "[Q] Send query to peers\n"
       << "[C] Check if a file exists\n"
       << "[L] Load a file\n"
       << "[H] Show this help again\n"
       << "[E] Exit the program\n====================\n";
  cout << "Hash is a 64 character string composed of only hexadecimal digits.\n"
       << "Example: "
          "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n============"
          "========\n";
}

int main() {
  json jsonData = readJson("./config.json");
  int port = jsonData["userPort"];

  string dir = jsonData["directory"];
  if (directoryCheck(dir) != 0) {
    return 1;
  }
  while (true) {
    help();
    string inputString;
    cin >> inputString;
    if (inputString.size() != 1) {
      cout << "====================\nUnrecognized input "
              "instruction\n====================\n";
      continue;
    }
    char input = inputString[0];
    if (input == 'Q' || input == 'q') {
      int socket = buildClient("127.0.0.1", to_string(port).c_str());
      query(socket, dir);
      close(socket);
    }
    else if (input == 'C' || input == 'c') {
      check(dir);
    }
    else if (input == 'L' || input == 'l') {
      cout << "Input the path to the file you want to upload:\n";
      string filepath;
      cin >> filepath;
      uploadFile(filepath, dir);
    }
    else if (input == 'E' || input == 'e') {
      cout << "Exiting client...\n";
      return 0;
    }
    else if (input == 'H' || input == 'h') {
      help();
    }
    else {
      cout << "====================\nUnrecognized input "
              "instruction\n====================\n";
      continue;
    }
  }
}
