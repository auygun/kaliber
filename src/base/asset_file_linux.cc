#include <string>
#include "asset_file.h"

namespace base {

AssetFile::AssetFile() = default;

AssetFile::~AssetFile() = default;

bool AssetFile::Open(const std::string& file_name,
                     const std::string& root_path) {
  std::string full_path = root_path + "assets/" + file_name;
  file_.reset(fopen(full_path.c_str(), "rb"));
  return !!file_;
}

void AssetFile::Close() {
  file_.reset();
}

int AssetFile::GetSize() {
  int size = 0;

  if (file_) {
    if (!fseek(file_.get(), 0, SEEK_END)) {
      size = ftell(file_.get());
      rewind(file_.get());
    }
  }

  return size;
}

int AssetFile::Read(char* data, size_t size) {
  if (file_)
    return fread(data, 1, size, file_.get());

  return 0;
}

}  // namespace base
