#include "file.h"
#include <assert.h>
#include "log.h"

char* File::ReadWholeFile(const char* file_name,
                          const char* root_path,
                          int* length,
                          bool null_terminate) {
  // Determine how big the file is.
  File file;
  if (file.Open(file_name, root_path)) {
    int size = file.GetSize();
    if (size) {
      // Allocate a new buffer and add space for a null terminator.
      char* buffer = new char[size + (null_terminate ? 1 : 0)];

      // Read all of it.
      int bytesRead = file.Read(buffer, size);
      if (!bytesRead) {
        LOG << "Failed to read a buffer of size: " << size << " from file " << file_name;
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
