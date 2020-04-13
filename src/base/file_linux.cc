#if defined(__linux__)

#include "file.h"
#include <string>

File::File()
  : file(NULL) {
}

File::~File() {
}

bool File::Open(const char *fileName) {
  std::string fullPath = rootPath;
  fullPath += fileName;

  file = fopen(fullPath.c_str(), "rb");
  return !!file;
}

bool File::Close() {
  bool result = false;
  if (file) {
    result = (0 == fclose(file));
    file = NULL;
  }
  return result;
}

unsigned File::GetSize() {
  unsigned size = 0;

  if (file) {
    if (!fseek(file, 0, SEEK_END)) {
      size = (unsigned)ftell(file);
      rewind(file);
    }
  }

  return size;
}

unsigned File::Read(char *data, unsigned size) {
  if (file)
    return (unsigned)fread(data, 1, size, file);

  return 0;
}

#endif // __linux__
