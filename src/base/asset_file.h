#ifndef ASSET_FILE_H
#define ASSET_FILE_H

#if defined(__ANDROID__)
#include <zlib.h>
#include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include <stdio.h>
#endif
#include <memory>
#include "file.h"

namespace base {

class AssetFile {
 public:
  AssetFile();
  ~AssetFile();

  bool Open(const char* file_name, const char* root_path);
  void Close();

  int GetSize();

  int Read(char* data, int size);

  static std::unique_ptr<char[]> ReadWholeFile(const char* file_name,
                                               const char* root_path,
                                               int* length = 0,
                                               bool null_terminate = false);

 private:
#if defined(__ANDROID__)
  unzFile archive_ = 0;
  int uncompressed_size_ = 0;
#elif defined(__linux)
  ScopedFILE file_;
#endif
};

} // namespace base

#endif  // ASSET_FILE_H
