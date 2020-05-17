#ifndef FILE_H
#define FILE_H

#if defined(__ANDROID__)
#include <zlib.h>
#include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include <stdio.h>
#endif
#include <memory>

namespace internal {

struct ScopedFILECloser {
  inline void operator()(FILE* x) const {
    if (x)
      fclose(x);
  }
};

} // namespace internal

// Automatically closes file.
using ScopedFILE = std::unique_ptr<FILE, internal::ScopedFILECloser>;

class File {
 public:
  File();
  ~File();

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

#endif  // FILE_H
