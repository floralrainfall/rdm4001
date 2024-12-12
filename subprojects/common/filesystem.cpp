#include "filesystem.hpp"

#include "logging.hpp"

namespace common {
static FileSystem* _singleton = 0;
FileSystem* FileSystem::singleton() {
  if (!_singleton) _singleton = new FileSystem();
  return _singleton;
}

FileSystem::FileSystem() {
  addApi(std::unique_ptr<FileSystemAPI>(new DataFolderAPI()));
}

FileSystemAPI* FileSystem::addApi(std::unique_ptr<FileSystemAPI> fapi,
                                  bool exclusive) {
  if (exclusive) fsApis.clear();
  fsApis.push_back(std::move(fapi));
  return fsApis.back().get();
};

OptionalData FileSystem::readFile(const char* path) {
  for (int i = fsApis.size() - 1; i >= 0; i--) {
    FileSystemAPI* api = fsApis[i].get();
    if (api->getFileExists(path)) {
      return api->getFileData(path);
    }
  }

  rdm::Log::printf(rdm::LOG_EXTERNAL, "Unable to load file %s", path);
  return {};
}

std::optional<FileIO*> FileSystem::getFileIO(const char* path,
                                             const char* mode) {
  for (int i = fsApis.size() - 1; i >= 0; i--) {
    FileSystemAPI* api = fsApis[i].get();
    if (api->getFileExists(path)) {
      return api->getFileIO(path, mode);
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
  FILE* fp = fopen((basedir + path).c_str(), "r");
  bool ex = fp;
  if (fp) fclose(fp);
  return ex;
}

OptionalData DataFolderAPI::getFileData(const char* path) {
  FILE* fp = fopen((basedir + path).c_str(), "rb");
  if (fp) {
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

std::optional<FileIO*> DataFolderAPI::getFileIO(const char* path,
                                                const char* mode) {
  FILE* fp = fopen((basedir + path).c_str(), mode);
  if (fp) {
    return new DataFileIO(fp);
  } else {
    return {};
  }
}

DataFileIO::DataFileIO(FILE* file) { this->file = file; }

DataFileIO::~DataFileIO() { fclose(file); }

size_t DataFileIO::seek(size_t pos, int whence) {
#ifdef DEBUG_DFIO
  rdm::Log::printf(rdm::LOG_DEBUG, "seek: %i, %i", pos, whence);
#endif
  fseek(file, pos, whence);
  return tell();
}

size_t DataFileIO::fileSize() {
  size_t cur = ftell(file);
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, cur, SEEK_SET);
#ifdef DEBUG_DFIO
  rdm::Log::printf(rdm::LOG_DEBUG, "size: %i", size);
#endif
  return size;
}

size_t DataFileIO::tell() {
#ifdef DEBUG_DFIO
  rdm::Log::printf(rdm::LOG_DEBUG, "tell: %i", ftell(file));
#endif
  return ftell(file);
}

size_t DataFileIO::read(void* out, size_t size) {
  size_t _read = fread(out, 1, size, file);
#ifdef DEBUG_DFIO
  rdm::Log::printf(rdm::LOG_DEBUG, "read: %p, %i (%i)", out, size, _read);
#endif
  return _read;
}

size_t DataFileIO::write(const void* in, size_t size) {
  return fwrite(in, size, 1, file);
}
};  // namespace common
