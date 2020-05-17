#include "file.h"
#include "log.h"

std::unique_ptr<char[]> File::ReadWholeFile(const char* file_name,
                                            const char* root_path,
                                            int* length,
                                            bool null_terminate) {
  // Determine how big the file is.
  File file;
  if (file.Open(file_name, root_path)) {
    int size = file.GetSize();
    if (size) {
      // Allocate a new buffer and add space for a null terminator.
      std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size + (null_terminate ? 1 : 0));

      // Read all of it.
      int bytesRead = file.Read(buffer.get(), size);
      if (!bytesRead) {
        LOG << "Failed to read a buffer of size: " << size << " from file " << file_name;
        return nullptr;
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

  return nullptr;
}
