#ifndef ASSET_FILE_H
#define ASSET_FILE_H

#if defined(__ANDROID__)
#include <zlib.h>
#include "../../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include "../../base/file.h"
#endif
#include <memory>
#include <string>

namespace eng {

class AssetFile {
 public:
  AssetFile();
  ~AssetFile();

  bool Open(const std::string& file_name, const std::string& root_path);
  void Close();

  size_t GetSize();

  size_t Read(char* data, size_t size);

  static std::unique_ptr<char[]> ReadWholeFile(const std::string& file_name,
                                               const std::string& root_path,
                                               size_t* length = 0,
                                               bool null_terminate = false);

 private:
#if defined(__ANDROID__)
  unzFile archive_ = 0;
  size_t uncompressed_size_ = 0;
#elif defined(__linux)
  base::ScopedFILE file_;
#endif
};

}  // namespace eng

#endif  // ASSET_FILE_H
