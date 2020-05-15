#if defined(__ANDROID__)

#include <assert.h>
#include <string>
#include "../platform/platform.h"
#include "file.h"
#include "log.h"

File::File() : archive_(0), uncompressed_size_(0) {}

File::~File() {
  Close();
}

bool File::Open(const char* file_name, const char* root_path) {
  do {
    // Try to open the zip archive.
    archive_ = unzOpen(root_path);
    if (!archive_) {
      LOG << "Failed to open zip file: " << root_path;
      break;
    }

    // Try to find the file.
    std::string fullName = "assets/";
    fullName += file_name;
    if (UNZ_OK != unzLocateFile(archive_, fullName.c_str(), 1)) {
      LOG << "Failed to locate file in zip archive: " << file_name;
      break;
    }

    // Need to get the uncompressed size of the file.
    unz_file_info info;
    if (UNZ_OK !=
        unzGetCurrentFileInfo(archive_, &info, NULL, 0, NULL, 0, NULL, 0)) {
      LOG << "Failed to get file info: " << file_name;
      break;
    }
    uncompressed_size_ = info.uncompressed_size;

    // Open the current file.
    if (UNZ_OK != unzOpenCurrentFile(archive_)) {
      LOG << "Failed to open file: " << file_name;
      break;
    }

    return true;
  } while (false);

  Close();
  return false;
}

bool File::Close() {
  if (archive_) {
    // This could potentially be called without having opened a file, but that
    // should be a harmless nop.
    unzCloseCurrentFile(archive_);

    unzClose(archive_);
    archive_ = 0;
  }

  return true;
}

unsigned File::GetSize() {
  return uncompressed_size_;
}

unsigned File::Read(char* data, unsigned size) {
  // Uncompress data into the buffer.
  int result = unzReadCurrentFile(archive_, data, size);
  return result < size ? 0 : result;
}

#endif  // __ANDROID__
