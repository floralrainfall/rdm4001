#pragma once
#include <vector>
#include <memory>
#include <optional>

namespace common {
  typedef std::optional<std::vector<unsigned char>> OptionalData;

  class FileSystemAPI {
  public:
    virtual bool getFileExists(const char* path) = 0;
    virtual OptionalData getFileData(const char* path) = 0;
  };

  class DataFolderAPI : public FileSystemAPI {
    std::string basedir;
  public:
    DataFolderAPI();
    virtual bool getFileExists(const char* path);
    virtual OptionalData getFileData(const char* path);
  };

  // abstraction
  class FileSystem {
    std::vector<std::unique_ptr<FileSystemAPI>> fsApis;

    FileSystem();
  public:
    static FileSystem* singleton();

    FileSystemAPI* addApi(std::unique_ptr<FileSystemAPI> api, bool exclusive = false);

    OptionalData readFile(const char* path);
  };
}