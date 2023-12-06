
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

// generate a cryptographically secure random number by reading urandom with max value
int genRandom(int max) {
  int rand;
  int urand = open("/dev/urandom", O_RDONLY);
  if (urand < 0) {
    std::cerr << "Error opening /dev/urandom\n";
    return -1;
  }
  if (read(urand, &rand, sizeof(int)) != sizeof(int)) {
    std::cerr << "Error reading /dev/urandom\n";
    return -1;
  }
  close(urand);

  // proportionally scale rand to be between 0 and max
  rand = (int)((double)rand / ((double)INT_MAX + 1) * max);
  return rand;
}
// math magic
int genKey(int a, int b, int c) {
  int res;
  int x = 1;
  while (b > 0) {
    res = b % 2;
    if (res == 1) {
      x = (x * a) % c;
    }
    a = (a * a) % c;
    b /= 2;
  }
  return res;
}

int gcm_encrypt(unsigned char * plaintext,
                int plaintext_len,
                unsigned char * aad,
                int aad_len,
                unsigned char * key,
                unsigned char * iv,
                int iv_len,
                unsigned char * ciphertext,
                unsigned char * tag) {
  EVP_CIPHER_CTX * ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handleErrors("Error creating context");
  }

  /* Initialise the encryption operation. */
  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
    handleErrors("Error initializing encryption");
  }

  /*
   * Set IV length if default 12 bytes (96 bits) is not appropriate
   */
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
    handleErrors("Error setting IV length");
  }

  /* Initialise key and IV */
  if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) {
    handleErrors("Error initializing key and IV");
  }

  /*
   * Provide any AAD data. This can be called zero or more times as
   * required
   */
  if (1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len)) {
    handleErrors("Error updating AAD");
  }

  /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
    handleErrors("Error updating ciphertext");
  }

  ciphertext_len = len;

  /*
   * Finalise the encryption. Normally ciphertext bytes may be written at
   * this stage, but this does not occur in GCM mode
   */
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
    handleErrors("Error finalizing encryption");
  }

  ciphertext_len += len;

  /* Get the tag */
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) {
    handleErrors("Error getting tag");
  }

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int gcm_decrypt(unsigned char * ciphertext,
                int ciphertext_len,
                unsigned char * aad,
                int aad_len,
                unsigned char * tag,
                unsigned char * key,
                unsigned char * iv,
                int iv_len,
                unsigned char * plaintext) {
  EVP_CIPHER_CTX * ctx;
  int len;
  int plaintext_len;
  int ret;

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handleErrors("Error creating context");
  }

  /* Initialise the decryption operation. */
  if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
    handleErrors("Error initializing decryption");
  }

  /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
  if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
    handleErrors("Error setting IV length");
  }

  /* Initialise key and IV */
  if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) {
    handleErrors("Error initializing key and IV");
  }
  /*
   * Provide any AAD data. This can be called zero or more times as
   * required
   */
  if (!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len)) {
    handleErrors("Error updating AAD");
  }

  /*
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
    handleErrors("Error updating ciphertext");
  }
  plaintext_len = len;

  /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
  if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
    handleErrors("Error setting tag");
  }

  /*
   * Finalise the decryption. A positive return value indicates success,
   * anything else is a failure - the plaintext is not trustworthy.
   */
  ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  if (ret > 0) {
    /* Success */
    plaintext_len += len;
    return plaintext_len;
  }
  else {
    /* Verify failed */
    return -1;
  }
}

/**
 * Encrypts a file using AES-256-GCM. The output is written to /tmp/encrypted.
 * writes tag after encryption. 
 * returns true if successful, else false.
*/
bool encrypt(std::string & inFileName,
             std::string & outFileName,
             unsigned char * key,
             unsigned char * tag,
             unsigned char * iv,
             int iv_len) {
  FILE * inputFile = fopen(inFileName.c_str(), "rb");
  FILE * outputFile = fopen(outFileName.c_str(), "wb");
  try {
    if (!inputFile) {
      throw std::runtime_error("Error: Unable to open input file.");
    }

    size_t fileSize = getFileLength(inFileName);

    unsigned char * plaintext = (unsigned char *)malloc(fileSize);
    fread(plaintext, 1, fileSize, inputFile);
    unsigned char ciphertext[fileSize];

    int ciphertext_len;

    /* Encrypt the plaintext */

    ciphertext_len =
        gcm_encrypt(plaintext, fileSize, NULL, 0, key, iv, iv_len, ciphertext, tag);

    // write to output file

    fwrite(ciphertext, 1, ciphertext_len, outputFile);
    fclose(inputFile);
    fclose(outputFile);
    return true;
  }
  catch (std::exception & e) {
    std::cout << e.what() << "\n";
    fclose(inputFile);
    fclose(outputFile);
    return false;
  }
}

/**
 * Decrypts a file using AES-256-GCM. The output is written to /tmp/decrypted.
 * returns true if successful, else false.
*/
bool decrypt(std::string & inFileName,
             std::string & outFileName,
             unsigned char * key,
             unsigned char * tag,
             unsigned char * iv,
             int iv_len) {
  FILE * inputFile = fopen(inFileName.c_str(), "rb");
  FILE * outputFile = fopen(outFileName.c_str(), "wb");
  try {
    if (!inputFile) {
      fprintf(stderr, "Error: Unable to open input file.\n");
      return false;
    }
    std::cout << "encrypted hash: " << getFileHash(inFileName) << "\n";

    size_t fileSize = getFileLength(inFileName);
    unsigned char decryptedtext[fileSize];
    char fileData[fileSize];
    fread(fileData, 1, fileSize, inputFile);

    // Calculate the length of the ciphertext
    long ciphertext_len = fileSize;

    unsigned char * ciphertext = (unsigned char *)malloc(ciphertext_len);
    memcpy(ciphertext, fileData, ciphertext_len);
    long decryptedtext_len;

    /* Decrypt the ciphertext */
    std::cout << ciphertext_len << std::endl;
    decryptedtext_len =
        gcm_decrypt(ciphertext, ciphertext_len, NULL, 0, tag, key, iv, 16, decryptedtext);

    if (decryptedtext_len <= 0) {
      throw std::runtime_error("decryption failed");
    }

    fwrite(decryptedtext, 1, decryptedtext_len, outputFile);
    fclose(inputFile);
    fclose(outputFile);
    return true;
  }
  catch (std::exception & e) {
    std::cout << e.what() << "\n";
    fclose(inputFile);
    fclose(outputFile);
    return false;
  }
}

// performs DH as alice. provides p and h. returns hex string of shared secret.
std::string alice(int fd) {
  EVP_PKEY * params = NULL;
  EVP_PKEY_CTX * kctx = NULL;
  EVP_PKEY * dhkey = NULL;
  EVP_PKEY * peerkey = NULL;
  DH * alice_dh = DH_get_2048_256();

  if (NULL == (params = EVP_PKEY_new())) {
    std::cout << "Error: EVP_PKEY_new failed. " << std::endl;
  }
  if (1 != EVP_PKEY_assign(params, EVP_PKEY_DHX, alice_dh)) {
    std::cout << "Error: EVP_PKEY_assign failed. " << std::endl;
  }
  if (!(kctx = EVP_PKEY_CTX_new(params, NULL))) {
    std::cout << "Error: EVP_PKEY_CTX_new failed. " << std::endl;
  }
  if (1 != EVP_PKEY_keygen_init(kctx)) {
    std::cout << "Error: EVP_PKEY_keygen_init failed. " << std::endl;
  }
  if (1 != EVP_PKEY_keygen(kctx, &dhkey)) {
    std::cout << "Error: EVP_PKEY_keygen failed. " << std::endl;
  }

  BIGNUM * p = BN_new();
  BIGNUM * b = BN_new();
  DH_get0_pqg(alice_dh, (const BIGNUM **)&p, NULL, (const BIGNUM **)&b);

  // send p and b to bob
  int len = BN_num_bytes(p);
  unsigned char * p_char = (unsigned char *)malloc(len);
  BN_bn2bin(p, p_char);

  send(fd, &len, sizeof(int), 0);
  send(fd, p_char, len, 0);

  len = BN_num_bytes(b);
  unsigned char * b_char = (unsigned char *)malloc(len);
  BN_bn2bin(b, b_char);

  send(fd, &len, sizeof(int), 0);
  send(fd, b_char, len, 0);

  // send alice's public key to bob
  BIGNUM * alice_pubkey = NULL;
  if (!EVP_PKEY_get_bn_param(dhkey, "pub", &alice_pubkey)) {
    std::cout << "Error: EVP_PKEY_get_bn_param failed. " << std::endl;
  }

  int alice_pub_key_size = BN_num_bytes(alice_pubkey);
  send(fd, &alice_pub_key_size, sizeof(int), 0);
  unsigned char * alice_pub_key_buffer = (unsigned char *)malloc(alice_pub_key_size);
  BN_bn2bin(alice_pubkey, alice_pub_key_buffer);
  send(fd, alice_pub_key_buffer, alice_pub_key_size, 0);

  // receive bob's public key

  int bob_key_size;
  if (recv(fd, &bob_key_size, sizeof(int), 0) <= 0) {
    std::cout << "Error: recv failed." << std::endl;
  }

  unsigned char * bob_pub_key_buffer = (unsigned char *)malloc(bob_key_size);
  if (recv(fd, bob_pub_key_buffer, bob_key_size, 0) <= 0) {
    std::cout << "Error: recv failed." << std::endl;
  }

  // Deserialize Bob's public key and set it in Bob's DH structure
  BIGNUM * bob_pub_key_bn = BN_bin2bn(bob_pub_key_buffer, bob_key_size, NULL);

  // print bob's public key

  // do before quitting

  BIGNUM * alice_privkey = NULL;
  if (!EVP_PKEY_get_bn_param(dhkey, "priv", &alice_privkey)) {
    std::cout << "Error: EVP_PKEY_get_bn_param failed. " << std::endl;
  }

  DH_set0_key(alice_dh, alice_pubkey, alice_privkey);

  unsigned char * shared_secret = NULL;
  size_t shared_secret_len = DH_size(alice_dh);

  std::stringstream ss;
  shared_secret = (unsigned char *)malloc(shared_secret_len);
  int shared_key_size = DH_compute_key(shared_secret, bob_pub_key_bn, alice_dh);
  if (shared_key_size <= 0) {
    std::cout << "Error: DH_compute_key failed." << std::endl;
  }
  else {
    for (int i = 0; i < shared_key_size; i++) {
      ss << std::hex << (int)shared_secret[i];
    }
  }

  DH_free(alice_dh);
  EVP_PKEY_free(dhkey);
  free(shared_secret);
  free((void *)alice_pub_key_buffer);
  free((void *)bob_pub_key_buffer);
  free((void *)p_char);
  free((void *)b_char);
  free((void *)params);
  free((void *)kctx);

  CONF_modules_unload(1);
  EVP_cleanup();

  ERR_free_strings();
  CRYPTO_cleanup_all_ex_data();
  return ss.str();
}

// performs DH as bob. receives p and h from alice. returns hex string of shared secret.
std::string bob(int fd) {
  int p_size;
  int b_size;

  recv(fd, &p_size, sizeof(int), 0);
  unsigned char * p_buffer = (unsigned char *)malloc(p_size);
  recv(fd, p_buffer, p_size, 0);

  recv(fd, &b_size, sizeof(int), 0);
  unsigned char * b_buffer = (unsigned char *)malloc(b_size);
  recv(fd, b_buffer, b_size, 0);

  BIGNUM *p, *b;
  p = BN_bin2bn(p_buffer, p_size, NULL);
  b = BN_bin2bn(b_buffer, b_size, NULL);

  // construct DH
  DH * bob_dh = DH_new();
  DH * alice_dh = DH_new();
  DH_set0_pqg(bob_dh, p, NULL, b);
  DH_set0_pqg(alice_dh, p, NULL, b);

  const BIGNUM * p_gen = BN_new();
  const BIGNUM * b_gen = BN_new();
  DH_get0_pqg(bob_dh, (const BIGNUM **)&p_gen, NULL, (const BIGNUM **)&b_gen);

  // generate key pair

  EVP_PKEY * params = EVP_PKEY_new();
  EVP_PKEY_CTX * kctx = NULL;
  EVP_PKEY * dhkey = NULL;
  EVP_PKEY * peerkey = NULL;

  if (NULL == params || 1 != EVP_PKEY_assign(params, EVP_PKEY_DHX, bob_dh)) {
    std::cout << "Error in EVP_PKEY initialization." << std::endl;
  }

  if (!(kctx = EVP_PKEY_CTX_new(params, NULL)) || 1 != EVP_PKEY_keygen_init(kctx) ||
      1 != EVP_PKEY_keygen(kctx, &dhkey)) {
    std::cout << "Error in key generation." << std::endl;
  }

  // receive alice's public key

  int alice_pub_key_size = 0;
  recv(fd, &alice_pub_key_size, sizeof(int), 0);

  unsigned char * alice_pub_key_buffer = (unsigned char *)malloc(alice_pub_key_size);

  recv(fd, alice_pub_key_buffer, alice_pub_key_size, 0);

  // Deserialize Alice's public key and set it in Bob's DH structure
  const BIGNUM * alice_pub_key_bn =
      BN_bin2bn(alice_pub_key_buffer, alice_pub_key_size, NULL);

  // send bob's public key to alice
  BIGNUM * bob_pubkey = NULL;
  if (!EVP_PKEY_get_bn_param(dhkey, "pub", &bob_pubkey)) {
    std::cout << "Error: EVP_PKEY_get_bn_param failed. " << std::endl;
  }

  int bob_pub_key_size = BN_num_bytes(bob_pubkey);
  send(fd, &bob_pub_key_size, sizeof(int), 0);

  unsigned char * bob_pub_key_buffer = (unsigned char *)malloc(bob_pub_key_size);
  BN_bn2bin(bob_pubkey, bob_pub_key_buffer);
  send(fd, bob_pub_key_buffer, bob_pub_key_size, 0);

  // initialze peerkey, store alice's public key, and derive shared secret
  BIGNUM * bob_privkey = NULL;
  if (!EVP_PKEY_get_bn_param(dhkey, "priv", &bob_privkey)) {
    std::cout << "Error: EVP_PKEY_get_bn_param failed. " << std::endl;
  }

  DH_set0_key(bob_dh, bob_pubkey, bob_privkey);

  unsigned char * shared_secret = NULL;
  size_t shared_secret_len = DH_size(bob_dh);
  shared_secret = (unsigned char *)malloc(shared_secret_len);
  int shared_key_size = DH_compute_key(shared_secret, alice_pub_key_bn, bob_dh);

  std::stringstream ss;
  if (shared_key_size <= 0) {
    std::cout << "Error: DH_compute_key failed." << std::endl;
  }
  else {
    for (int i = 0; i < shared_key_size; i++) {
      ss << std::hex << (int)shared_secret[i];
    }
  }

  std::cout << ss.str() << std::endl;
  // hash shared secret sha256

  // Free the allocated memory

  // cleanup
  DH_free(alice_dh);
  EVP_PKEY_free(dhkey);
  free(shared_secret);
  free((void *)alice_pub_key_buffer);
  free((void *)bob_pub_key_buffer);

  free((void *)params);
  free((void *)kctx);

  free(p_buffer);
  free(b_buffer);

  CONF_modules_unload(1);
  EVP_cleanup();

  ERR_free_strings();
  CRYPTO_cleanup_all_ex_data();
  return ss.str();
}
