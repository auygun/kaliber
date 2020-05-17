#ifndef FILE_H
#define FILE_H

#if defined(__ANDROID__)
#include <zlib.h>
#include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
#include <stdio.h>
#endif
#include <memory>

class File {
 public:
  File();
  ~File();

  bool Open(const char* file_name, const char* root_path);
  bool Close();

  int GetSize();

  int Read(char* data, int size);

  static char* ReadWholeFile(const char* file_name,
                             const char* root_path,
                             int* length = 0,
                             bool null_terminate = false);

 private:
#if defined(__ANDROID__)
  unzFile archive_;
  int uncompressed_size_;
#elif defined(__linux)
  FILE* file_;
#endif
};

#endif  // FILE_H
