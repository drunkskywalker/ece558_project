#include <openssl/evp.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

/*
g++ -o testhash hash.cpp -lssl -lcrypto

*/
void handleErrors() {
    std::cout << "ERR\n";
}

// helper function to calculate sha256 hash.
void digest_message(const unsigned char *message,
                    size_t message_len,
                    unsigned char **digest,
                    unsigned int *digest_len) {
    EVP_MD_CTX *mdctx;

    if ((mdctx = EVP_MD_CTX_new()) == NULL) {
        handleErrors();
    }
    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
        handleErrors();
    }

    if (1 != EVP_DigestUpdate(mdctx, message, message_len)) {
        handleErrors();
    }

    if ((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL) {
        handleErrors();
    }

    if (1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len)) {
        handleErrors();
    }

    EVP_MD_CTX_free(mdctx);
}

/*
filename is the filename to hash.
returns the sha256 hash of filename.
*/
std::string getFileHash(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return "";
    }

    std::ostringstream fileContent;
    fileContent << file.rdbuf();
    file.close();

    const std::string &content = fileContent.str();
    const unsigned char *message = (const unsigned char *)(content.c_str());
    size_t message_len = content.size();

    unsigned char *digest;
    unsigned int digest_len;

    digest_message(message, message_len, &digest, &digest_len);

    std::ostringstream hashString;
    hashString << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        hashString << std::setw(2) << static_cast<int>(digest[i]);
    }

    OPENSSL_free(digest);
    return hashString.str();
}

/*
directory is the directory to get all files.
returns a vector of filenames in the directory.
*/
std::vector<std::string> allFiles(const std::string &directory) {
    std::vector<std::string> files;

    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

/*
hash is the sha256 hash to check against. filename is the file to check.
returns true if filename hashes to hash. else returns false.
*/
bool match(std::string hash, std::string filename) {
    std::string fileHash = getFileHash(filename);
    return hash.compare(fileHash) == 0;
}

int main() {
    std::vector<std::string> files = allFiles(".");
    for (int i = 0; i < files.size(); i++) {
        std::cout << files.at(i) << ": " << getFileHash(files.at(i)) << "\n";
    }
    return 0;
}
