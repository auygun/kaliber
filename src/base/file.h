#ifndef FILE_H
#define FILE_H

#if defined(__ANDROID__)
# include <zlib.h>
# include "../third_party/minizip/unzip.h"
#elif defined(__linux__)
# include <stdio.h>
#endif

class File {
public:
  File();
  ~File();

  bool Open(const char *file_name);
  bool Close();

  unsigned GetSize();

  unsigned Read(char *data, unsigned size);

  static char *ReadWholeFile(const char *file_name, unsigned *length = 0,
                             bool null_terminate = false);

private:
#if defined(__ANDROID__)
  unzFile   archive_;
  unsigned  uncompressed_size_;
#elif defined(__linux)
  FILE      *file_;
#endif
};

#endif // FILE_H
