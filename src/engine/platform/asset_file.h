#ifndef ENGINE_PLATFORM_ASSET_FILE_H
#define ENGINE_PLATFORM_ASSET_FILE_H

#include <memory>
#include <string>

#if defined(__ANDROID__)
#include <zlib.h>
#include "third_party/minizip/unzip.h"
#else
#include "base/file.h"
#endif

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
#else
  base::ScopedFILE file_;
#endif
};

}  // namespace eng

#endif  // ENGINE_PLATFORM_ASSET_FILE_H
