
#include "hash.hpp"
#define RECURSIVE 1

void handleErrors(std::string msg) {
  throw std::runtime_error(msg);
}

// helper function to calculate sha256 hash.
void digest_message(const unsigned char * message,
                    size_t message_len,
                    unsigned char ** digest,
                    unsigned int * digest_len) {
  EVP_MD_CTX * mdctx;

  if ((mdctx = EVP_MD_CTX_new()) == NULL) {
    handleErrors("Error creating context");
  }
  if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
    handleErrors("Error initializing digest");
  }

  if (1 != EVP_DigestUpdate(mdctx, message, message_len)) {
    handleErrors("Error updating digest");
  }

  if ((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL) {
    handleErrors("Error allocating digest");
  }

  if (1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len)) {
    handleErrors("Error finalizing digest");
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

std::string getVectorCharHash(const std::vector<char> & content) {
  const unsigned char * message = (const unsigned char *)content.data();
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
  FILE * fptr = fopen(filename.c_str(), "rb");
  if (fptr == NULL) {
    std::cout << "Failed to open file " << filename << "\n";
    return -1;
  }
  // get file size
  fseek(fptr, 0, SEEK_END);
  return ftell(fptr);
}

/*
directory is the directory to get all files.
returns a vector of filenames in the directory.
*/

std::vector<std::string> allFiles(const std::string & directory, bool recursive) {
  std::vector<std::string> files;
  DIR * dir;
  struct dirent * ent;
  if ((dir = opendir(directory.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_type == DT_REG) {
        files.push_back(directory + "/" + ent->d_name);
      }

      if (ent->d_type == DT_DIR && recursive) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
          std::vector<std::string> sub =
              allFiles(directory + "/" + ent->d_name, recursive);
          for (size_t i = 0; i < sub.size(); i++) {
            files.push_back(sub[i]);
          }
        }
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
  std::vector<std::string> files = allFiles(directory, RECURSIVE);
  for (size_t i = 0; i < files.size(); i++) {
    if (matchHash(hash, files[i])) {
      return true;
    }
  }
  return false;
}

std::string findFileName(std::string & hash, std::string & directory) {
  std::vector<std::string> files = allFiles(directory, RECURSIVE);
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

std::string getFileNameFromPath(std::string & path) {
  size_t pos = path.find_last_of("/");
  return path.substr(pos + 1);
}
