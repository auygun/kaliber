#if defined(__ANDROID__)

#include "file.h"
#include "log.h"
#include <assert.h>
#include <string>

File::File()
  : archive(0)
  , uncompressedSize(0) {
}

File::~File() {
  Close();
}

bool File::Open(const char *fileName) {
  do {
    // Try to open the zip archive.
    archive = unzOpen(rootPath);
    if (!archive) {
      LOG("Failed to open zip file: %s\n", rootPath);
      break;
    }

    // Try to find the file.
    std::string fullName = "assets/";
    fullName += fileName;
    if (UNZ_OK != unzLocateFile(archive, fullName.c_str(), 1)) {
      LOG("Failed to locate file in zip archive: %s\n", fileName);
      break;
    }

    // Need to get the uncompressed size of the file.
    unz_file_info info;
    if (UNZ_OK != unzGetCurrentFileInfo(archive, &info, NULL, 0, NULL, 0, NULL,
                                        0)) {
      LOG("Failed to get file info: %s\n", fileName);
      break;
    }
    uncompressedSize = info.uncompressed_size;

    // Open the current file.
    if (UNZ_OK != unzOpenCurrentFile(archive)) {
      LOG("Failed to open file: %s\n", fileName);
      break;
    }

    return true;
  }
  while (false);

  Close();
  return false;
}

bool File::Close() {
  if (archive) {
    // This could potentially be called without having opened a file, but that
    // should be a harmless nop.
    unzCloseCurrentFile(archive);

    unzClose(archive);
    archive = 0;
  }

  return true;
}

unsigned File::GetSize() {
  return uncompressedSize;
}

unsigned File::Read(char *data, unsigned size) {
  // Uncompress data into the buffer.
  int result = unzReadCurrentFile(archive, data, size);
  return result < size ? 0 : result;
}

#endif // __ANDROID__
