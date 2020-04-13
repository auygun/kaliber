#include "file.h"
#include "log.h"
#include <assert.h>

const char *File::rootPath = "../../assets/";

char *File::ReadWholeFile(const char *fileName, unsigned *length,
                          bool nullTerminate) {
  // Determine how big the file is.
  File file;
  if (file.Open(fileName)) {
    unsigned size = file.GetSize();
    if (size) {
      // Allocate a new buffer and add space for a null terminator.
      char *buffer = new char [size + (nullTerminate ? 1 : 0)];

      // Read all of it.
      unsigned bytesRead = file.Read(buffer, size);
      if (!bytesRead) {
        LOG("Failed to read a buffer of size %d from file %s\n", (int)size, fileName);
        delete [] buffer;
        return NULL;
      }

      // Return the buffer size if the caller is interested.
      if (length)
        *length = bytesRead;

      // Null terminate the buffer.
      if (nullTerminate)
        buffer[size] = 0;

      return buffer;
    }
  }

  return NULL;
}
