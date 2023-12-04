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
    std::cerr << "Error opening file\n";
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
    std::cout << "Successfully uploaded.\n";
  }
  else {
    std::cerr << "Error executing command.\n";
  }
  return result;
}

void query(int & socket, string & dir) {
  cout << "What file do you want to check, input hash: " << endl;
  string hash;
  cin >> hash;

  if (checkValidSHA256(hash)) {
    if (checkFileExist(hash, dir)) {
      cout << "File already exists." << endl;
    }
    else {
      cout << "File does not exist. sending query..." << endl;
      sendQuery(socket, hash);
    }
  }
  else {
    cout << "Hash is not valid" << endl;
  }
}

void check(string & dir) {
  cout << "What file do you want to check, input hash: " << endl;
  string hash;
  cin >> hash;

  if (checkValidSHA256(hash)) {
    if (checkFileExist(hash, dir)) {
      cout << "File exists." << endl;
    }
    else {
      cout << "File does not exist." << endl;
    }
  }
  else {
    cout << "Hash is not valid." << endl;
  }
}

int directoryCheck(string & dir) {
  DIR * directory;
  if ((directory = opendir(dir.c_str())) == NULL) {
    cout << "Directory " << dir << " does not exist. Creating directory...\n";
    if (mkdir(dir.c_str(), 0777) != 0) {
      cout << "Error creating directory. Check if you are permitted to do so.\n";
      return 1;
    }
    else {
      cout << "Successfully created directory.\n";
    }
  }
  return 0;
}

void help() {
  cout << "What do you want to do?\n"
       << "Q: Send query to peers\n"
       << "C: Check if a file exists\n"
       << "L: Load a file\n"
       << "H: Show this help\n"
       << "E: Exit the program\n\n";
  cout << "Hash is a 64 character string composed of only hexadecimal digits.\n"
       << "Example: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n";
}

int main() {
  json jsonData = readJson("./config.json");
  int port = jsonData["userPort"];
  int socket = buildClient("127.0.0.1", to_string(port).c_str());
  string dir = jsonData["directory"];
  if (directoryCheck(dir) != 0) {
    return 1;
  }
  while (true) {
    help();
    string inputString;
    cin >> inputString;
    if (inputString.size() != 1) {
      cout << "Unrecognized input instruction\n";
      continue;
    }
    char input = inputString[0];
    if (input == 'Q' || input == 'q') {
      query(socket, dir);
    }
    else if (input == 'C' || input == 'c') {
      check(dir);
    }
    else if (input == 'L' || input == 'l') {
      cout << "What file do you want to upload, input path to file\n";
      string filepath;
      cin >> filepath;
      uploadFile(filepath, dir);
    }
    else if (input == 'E' || input == 'e') {
      cout << "Exiting client..." << endl;
      return 0;
    }
    else if (input == 'H' || input == 'h') {
      help();
    }
    else {
      cout << "Unrecognized input instruction\n";
      continue;
    }
  }
}
