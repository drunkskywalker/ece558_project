#include <dirent.h>
#include <openssl/evp.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

void handleErrors();
void digest_message(const unsigned char * message,
                    size_t message_len,
                    unsigned char ** digest,
                    unsigned int * digest_len);
std::string getFileHash(const std::string & filename);
std::vector<std::string> allFiles(const std::string & directory);
bool matchHash(std::string & hash, std::string & filename);
bool checkFileExist(std::string & hash, std::string & directory);

bool checkValidSHA256(std::string & hash);