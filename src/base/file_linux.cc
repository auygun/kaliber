#include <string>
#include "file.h"

File::File() = default;

File::~File() = default;

bool File::Open(const char* file_name, const char* root_path) {
  std::string full_path = root_path;
  full_path += file_name;
  file_.reset(fopen(full_path.c_str(), "rb"));
  return !!file_;
}

void File::Close() {
  file_.reset();
}

int File::GetSize() {
  int size = 0;

  if (file_) {
    if (!fseek(file_.get(), 0, SEEK_END)) {
      size = ftell(file_.get());
      rewind(file_.get());
    }
  }

  return size;
}

int File::Read(char* data, int size) {
  if (file_)
    return fread(data, 1, size, file_.get());

  return 0;
}
