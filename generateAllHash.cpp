#include "hash.cpp"

// g++ -o generateAllHash generateAllHash.cpp -lssl -lcrypto

int main() {
  std::vector<std::string> pathes = {
      "./example/folder1", "./example/folder2", "./example/folder3", "./example/folder4"};

  for (auto path : pathes) {
    std::string pt = path + "/hashes.txt";
    remove(pt.c_str());
    std::vector<std::string> hashes = allFiles(path);
    FILE * fp = fopen(pt.c_str(), "wb");

    for (auto p : hashes) {
      fprintf(fp, "%s:", p.c_str());
      std::string hash = getFileHash(p);
      fprintf(fp, "%s\n", hash.c_str());
    }
    fclose(fp);
  }

  return 0;
}