#include "engine/platform/asset_file.h"

#include "base/log.h"

namespace eng {

AssetFile::AssetFile() = default;

AssetFile::~AssetFile() {
  Close();
}

bool AssetFile::Open(const std::string& file_name,
                     const std::string& root_path) {
  do {
    // Try to open the zip archive.
    archive_ = unzOpen(root_path.c_str());
    if (!archive_) {
      LOG(0) << "Failed to open zip file: " << root_path;
      break;
    }

    // Try to find the file.
    std::string full_name = "assets/" + file_name;
    if (UNZ_OK != unzLocateFile(archive_, full_name.c_str(), 1)) {
      LOG(0) << "Failed to locate file in zip archive: " << file_name;
      break;
    }

    // Need to get the uncompressed size of the file.
    unz_file_info info;
    if (UNZ_OK !=
        unzGetCurrentFileInfo(archive_, &info, NULL, 0, NULL, 0, NULL, 0)) {
      LOG(0) << "Failed to get file info: " << file_name;
      break;
    }
    uncompressed_size_ = info.uncompressed_size;

    // Open the current file.
    if (UNZ_OK != unzOpenCurrentFile(archive_)) {
      LOG(0) << "Failed to open file: " << file_name;
      break;
    }

    return true;
  } while (false);

  Close();
  return false;
}

void AssetFile::Close() {
  if (archive_) {
    // This could potentially be called without having opened a file, but that
    // should be a harmless nop.
    unzCloseCurrentFile(archive_);

    unzClose(archive_);
    archive_ = 0;
  }
}

size_t AssetFile::GetSize() {
  return uncompressed_size_;
}

size_t AssetFile::Read(char* data, size_t size) {
  // Uncompress data into the buffer.
  int result = unzReadCurrentFile(archive_, data, size);
  return result < 0 ? 0 : result;
}

}  // namespace eng
