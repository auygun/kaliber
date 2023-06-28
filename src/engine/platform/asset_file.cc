#include "engine/platform/asset_file.h"

#include "base/log.h"

namespace eng {

std::unique_ptr<char[]> AssetFile::ReadWholeFile(const std::string& file_name,
                                                 const std::string& root_path,
                                                 size_t* length,
                                                 bool null_terminate) {
  AssetFile file;
  if (!file.Open(file_name, root_path))
    return nullptr;

  size_t size = file.GetSize();
  if (size == 0)
    return nullptr;

  // Allocate a new buffer and add space for a null terminator.
  std::unique_ptr<char[]> buffer =
      std::make_unique<char[]>(size + (null_terminate ? 1 : 0));

  // Read all of it.
  size_t bytes_read = file.Read(buffer.get(), size);
  if (!bytes_read) {
    LOG(0) << "Failed to read a buffer of size: " << size << " from file "
           << file_name;
    return nullptr;
  }

  // Return the buffer size if the caller is interested.
  if (length)
    *length = bytes_read;

  // Null terminate the buffer.
  if (null_terminate)
    buffer[size] = 0;

  return buffer;
}

}  // namespace eng
