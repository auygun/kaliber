#include "asset_file.h"
#include "log.h"

namespace base {

std::unique_ptr<char[]> AssetFile::ReadWholeFile(const std::string& file_name,
                                                 const std::string& root_path,
                                                 size_t* length,
                                                 bool null_terminate) {
  AssetFile file;
  if (!file.Open(file_name, root_path))
    return nullptr;

  int size = file.GetSize();
  if (size == 0)
    return nullptr;

  // Allocate a new buffer and add space for a null terminator.
  std::unique_ptr<char[]> buffer =
      std::make_unique<char[]>(size + (null_terminate ? 1 : 0));

  // Read all of it.
  int bytesRead = file.Read(buffer.get(), size);
  if (!bytesRead) {
    LOG << "Failed to read a buffer of size: " << size << " from file "
        << file_name;
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

}  // namespace base
