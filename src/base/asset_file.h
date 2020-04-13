#ifndef ASSET_FILE_H
#define ASSET_FILE_H

#include "file.h"
#if defined(__ANDROID__)
#include <zlib.h>
#include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include <stdio.h>
#endif
#include <memory>
#include <string>

namespace base {

class AssetFile {
 public:
  AssetFile();
  ~AssetFile();

  bool Open(const std::string& file_name, const std::string& root_path);
  void Close();

  int GetSize();

  int Read(char* data, size_t size);

  static std::unique_ptr<char[]> ReadWholeFile(const std::string& file_name,
                                               const std::string& root_path,
                                               size_t* length = 0,
                                               bool null_terminate = false);

 private:
#if defined(__ANDROID__)
  unzFile archive_ = 0;
  size_t uncompressed_size_ = 0;
#elif defined(__linux)
  ScopedFILE file_;
#endif
};

}  // namespace base

#endif  // ASSET_FILE_H
