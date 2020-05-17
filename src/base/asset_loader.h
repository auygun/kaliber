#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#if defined(__ANDROID__)
#include <zlib.h>
#include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include <stdio.h>
#endif
#include <memory>
#include "file.h"

class AssetLoader {
 public:
  AssetLoader();
  ~AssetLoader();

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

#endif  // ASSET_LOADER_H
