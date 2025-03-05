#pragma once
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace common {
typedef std::optional<std::vector<unsigned char>> OptionalData;

class FileIO {
 public:
  FileIO() {};
  virtual ~FileIO() {};

  virtual size_t seek(size_t pos, int whence) = 0;
  virtual size_t fileSize() = 0;
  virtual size_t tell() = 0;

  virtual size_t read(void* out, size_t size) = 0;
  virtual size_t write(const void* in, size_t size) = 0;
};

class FileSystemAPI {
 public:
  // override this to return false if you don't want the engine to test if files
  // exist here, useful for HTTP or web retrevial fs apis
  virtual bool generalFSApi() { return true; }
  virtual bool getFileExists(const char* path) = 0;
  virtual OptionalData getFileData(const char* path) = 0;
  virtual std::optional<FileIO*> getFileIO(const char* path,
                                           const char* mode) = 0;
};

class DataFileIO : public FileIO {
  FILE* file;

 public:
  DataFileIO(FILE* file);
  virtual ~DataFileIO();

  virtual size_t seek(size_t pos, int whence);
  virtual size_t fileSize();
  virtual size_t tell();

  virtual size_t read(void* out, size_t size);
  virtual size_t write(const void* in, size_t size);
};

class DataFolderAPI : public FileSystemAPI {
  std::string basedir;

  void checkProperDir(const char* path);

 public:
  DataFolderAPI(std::string basedir = "");

  virtual bool getFileExists(const char* path);
  virtual OptionalData getFileData(const char* path);
  virtual std::optional<FileIO*> getFileIO(const char* path, const char* mode);
};

// abstraction
class FileSystem {
  struct FSApiInfo {
    std::unique_ptr<FileSystemAPI> api;
    int precedence;
  };

  std::map<std::string, FSApiInfo> fsApis;

  FileSystemAPI* getOwningApi(const char* path);
  std::string sanitizePath(const char* path);
  FileSystem();

 public:
  static FileSystem* singleton();

  // higher numbers take precedence
  void addApi(FileSystemAPI* api, std::string uri, int precedence = 0,
              bool exclusive = false);

  OptionalData readFile(const char* path);
  std::optional<FileIO*> getFileIO(const char* path, const char* mode);
};
}  // namespace common
