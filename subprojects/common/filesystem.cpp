#include "filesystem.hpp"
#include "logging.hpp"

namespace common {
static FileSystem* _singleton = 0;
FileSystem* FileSystem::singleton() {
  if(!_singleton)
    _singleton = new FileSystem();
  return _singleton;
}

FileSystem::FileSystem() {
  addApi(std::unique_ptr<FileSystemAPI>(new DataFolderAPI()));
}

FileSystemAPI* FileSystem::addApi(std::unique_ptr<FileSystemAPI> fapi, bool exclusive) {
  if(exclusive)
    fsApis.clear();
  fsApis.push_back(std::move(fapi));
  return fsApis.back().get();
};

OptionalData FileSystem::readFile(const char* path) {
  for(int i = fsApis.size()-1; i >= 0; i--) {
    FileSystemAPI* api = fsApis[i].get();
    if(api->getFileExists(path)) {
      return api->getFileData(path);
    }
  }

  rdm::Log::printf(rdm::LOG_EXTERNAL, "Unable to load file %s", path);
  return {};
}

DataFolderAPI::DataFolderAPI() {
#ifndef NDEBUG
  basedir = "../data/";
#else
  basedir = "data/";
#endif
}

bool DataFolderAPI::getFileExists(const char* path) {
  FILE* fp = fopen((basedir + path).c_str(),"r");
  bool ex = fp;
  if(fp)
    fclose(fp);
  return ex;
}

OptionalData DataFolderAPI::getFileData(const char* path) {
  FILE* fp = fopen((basedir + path).c_str(),"r");
  if(fp) {
    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<unsigned char> v(sz);
    fread(v.data(), sz, 1, fp);
    fclose(fp);
    return v;
  } else {
    return {};
  }
}
};
