#include "file.h"
#include <assert.h>
#include "log.h"

char* File::ReadWholeFile(const char* file_name,
                          unsigned* length,
                          bool null_terminate) {
  // Determine how big the file is.
  File file;
  if (file.Open(file_name)) {
    unsigned size = file.GetSize();
    if (size) {
      // Allocate a new buffer and add space for a null terminator.
      char* buffer = new char[size + (null_terminate ? 1 : 0)];

      // Read all of it.
      unsigned bytesRead = file.Read(buffer, size);
      if (!bytesRead) {
        LOG << "Failed to read a buffer of size: " << (int)size << " from file " << file_name;
        delete[] buffer;
        return NULL;
      }

      // Return the buffer size if the caller is interested.
      if (length)
        *length = bytesRead;

      // Null terminate the buffer.
      if (null_terminate)
        buffer[size] = 0;

      return buffer;
    }
  }

  return NULL;
}
