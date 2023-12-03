
#include "hash.hpp"

void handleErrors() {
  std::cout << "Error hashing file\n";
}

// helper function to calculate sha256 hash.
void digest_message(const unsigned char * message,
                    size_t message_len,
                    unsigned char ** digest,
                    unsigned int * digest_len) {
  EVP_MD_CTX * mdctx;

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
std::string getFileHash(const std::string & filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return "";
  }

  std::ostringstream fileContent;
  fileContent << file.rdbuf();
  file.close();

  const std::string & content = fileContent.str();
  const unsigned char * message = (const unsigned char *)(content.c_str());
  size_t message_len = content.size();

  unsigned char * digest;
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

size_t getFileLength(std::string & filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return 0;
  }
  file.seekg(0, std::ios::end);
  size_t length = file.tellg();
  file.close();

  return length;
}

/*
directory is the directory to get all files.
returns a vector of filenames in the directory.
*/

std::vector<std::string> allFiles(const std::string & directory) {
  std::vector<std::string> files;
  DIR * dir;
  struct dirent * ent;
  if ((dir = opendir(directory.c_str())) != NULL) {
    while ((ent = readdir(dir)) != nullptr) {
      if (ent->d_type == DT_REG) {
        files.push_back(directory + "/" + ent->d_name);
      }
    }
    closedir(dir);
  }
  else {
    std::cout << "Error opening directory\n";
  }

  return files;
}
/*
hash is the sha256 hash to check against. filename is the file to check.
returns true if filename hashes to hash. else returns false.
*/
bool matchHash(std::string & hash, std::string & filename) {
  std::string fileHash = getFileHash(filename);
  return hash.compare(fileHash) == 0;
}

bool checkFileExist(std::string & hash, std::string & directory) {
  std::vector<std::string> files = allFiles(directory);
  for (size_t i = 0; i < files.size(); i++) {
    if (matchHash(hash, files[i])) {
      return true;
    }
  }
  return false;
}

std::string findFileName(std::string & hash, std::string & directory) {
  std::vector<std::string> files = allFiles(directory);
  for (size_t i = 0; i < files.size(); i++) {
    if (matchHash(hash, files[i])) {
      return files[i];
    }
  }
  return "";
}

bool checkValidSHA256(std::string & hash) {
  if (hash.size() != 64) {
    return false;
  }
  for (size_t i = 0; i < hash.size(); i++) {
    if (!isxdigit(hash[i])) {
      return false;
    }
  }
  return true;
}

ContentMeta genContentMeta(std::string & hash, std::string & dir) {
  ContentMeta meta;
  if (checkFileExist(hash, dir)) {
    std::string filename = findFileName(hash, dir);
    meta.status = true;
    strcpy(meta.fileName, filename.c_str());
    meta.length = getFileLength(filename);
  }
  else {
    meta.status = false;
    meta.fileName[0] = '\0';
    meta.length = 0;
  }

  return meta;
}
