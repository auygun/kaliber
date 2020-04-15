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

  bool Open(const char *fileName);
  bool Close();

  unsigned GetSize();

  unsigned Read(char *data, unsigned size);

  static char *ReadWholeFile(const char *fileName, unsigned *length = 0,
                             bool nullTerminate = false);

private:
#if defined(__ANDROID__)
  unzFile   archive;
  unsigned  uncompressedSize;
#elif defined(__linux)
  FILE      *file;
#endif
};

#endif // FILE_H
