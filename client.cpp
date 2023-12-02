#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include "socketUtils.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include "hash.hpp"

using json = nlohmann::json;
using namespace std;

json readJson(string filename){
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

    // // Access data from the JSON object
    // int port  = jsonData["userPort"];
    return jsonData;
}

int sendQuery(int socket, string & hash){
    int status = send(socket, hash.c_str(), 64, 0);
    errorHandle(status,"Error: send hash failed",NULL, NULL);
    return status; 
}

int uploadFile(string & path, string & directory){
    string command = "cp " + path + " " + directory;
    int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "Successfully uploaded.\n";
    } else {
        std::cerr << "Error executing command.\n";
    }
    return result; 
}
int main(){
    json jsonData = readJson("config.json");
    int port = jsonData["userPort"];
    // cout<< port ;
    // int socket = buildClient("127.0.0.1",to_string(port).c_str());
    string dir = jsonData["directory"];

    while(true){
        cout << "what do you want to do?" << endl;
        cout << "[Q] send query to peers\n[C] check if a file exists\n[L] load a file\n[E] exit the program " << endl;
        string inputString;
        cin >> inputString;
        if(inputString.size() != 1){
            cout<<"unrecognized input instruction!!!!"<< endl;
            continue;
        }
        char input = inputString[0];
        

        if (input == 'Q'){
            cout<<"What file do you want to query, input hash: " << endl;
            string hash;
            cin >> hash;
            if (checkValidSHA256(hash)){
                // sendQuery(socket, hash);
            }else{
                cout<<"Hash is not valid"<< endl;
                continue;
            }
        }else if(input == 'C'){
            cout << "What file do you want to check, input hash: " << endl;
            string hash;
            cin >> hash;
            
            if (checkValidSHA256(hash)){
                if(checkFileExist(hash, dir)){
                    cout<<"File EXISTS"<< endl;
                }else{
                    cout<<"File DOES NOT EXISTS"<< endl;
                }
            }else{
                cout<<"Hash is not valid"<< endl;
                continue;
            }
        }else if(input == 'L'){
            cout << "What file do you want to upload, input path to file" << endl;
            string filepath;
            cin >> filepath;
            uploadFile(filepath, dir);
        }else if(input == 'E'){
            cout << "finished!"<<endl;
            return 0;
        }else{
            cout<<"unrecognized input instruction!!!!"<< endl;
            continue;
        }
        // send query, check if exist, exit

    }
    

}
