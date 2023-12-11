#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/conf.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "message.hpp"

void handleErrors(std::string msg);
void digest_message(const unsigned char * message,
                    size_t message_len,
                    unsigned char ** digest,
                    unsigned int * digest_len);

std::string getFileHash(const std::string & filename);
std::string getVectorCharHash(const std::vector<char> & content);
size_t getFileLength(std::string & filename);
std::vector<std::string> allFiles(const std::string & directory, bool recursive);
bool matchHash(std::string & hash, std::string & filename);
bool checkFileExist(std::string & hash, std::string & directory);
std::string findFileName(std::string & hash, std::string & directory);
bool checkValidSHA256(std::string & hash);

std::string getFileNameFromPath(std::string & path);

int genRandom(int max);
int genKey(int a, int b, int c);

int gcm_encrypt(unsigned char * plaintext,
                int plaintext_len,
                unsigned char * aad,
                int aad_len,
                unsigned char * key,
                unsigned char * iv,
                int iv_len,
                unsigned char * ciphertext,
                unsigned char * tag);

int gcm_decrypt(unsigned char * ciphertext,
                int ciphertext_len,
                unsigned char * aad,
                int aad_len,
                unsigned char * tag,
                unsigned char * key,
                unsigned char * iv,
                int iv_len,
                unsigned char * plaintext);

bool encrypt(std::string & inFileName,
             std::string & outFileName,
             unsigned char * key,
             unsigned char * tag,
             unsigned char * iv,
             int iv_len);

bool decrypt(std::string & inFileName,
             std::string & outFileName,
             unsigned char * key,
             unsigned char * tag,
             unsigned char * iv,
             int iv_len);

std::string alice(int fd);
std::string bob(int fd);