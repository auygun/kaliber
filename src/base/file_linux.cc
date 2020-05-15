#if defined(__linux__)

#include <string>
#include "../platform/platform.h"
#include "file.h"

File::File() : file_(NULL) {}

File::~File() {}

bool File::Open(const char* file_name, const char* root_path) {
  std::string full_path = root_path;
  full_path += file_name;
  file_ = fopen(full_path.c_str(), "rb");
  return !!file_;
}

bool File::Close() {
  bool result = false;
  if (file_) {
    result = (0 == fclose(file_));
    file_ = NULL;
  }
  return result;
}

unsigned File::GetSize() {
  unsigned size = 0;

  if (file_) {
    if (!fseek(file_, 0, SEEK_END)) {
      size = (unsigned)ftell(file_);
      rewind(file_);
    }
  }

  return size;
}

unsigned File::Read(char* data, unsigned size) {
  if (file_)
    return (unsigned)fread(data, 1, size, file_);

  return 0;
}

#endif  // __linux__
