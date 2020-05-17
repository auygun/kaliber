#ifndef FILE_H
#define FILE_H

#include <memory>

namespace internal {

struct ScopedFILECloser {
  inline void operator()(FILE* x) const {
    if (x)
      fclose(x);
  }
};

} // namespace internal

// Automatically closes file.
using ScopedFILE = std::unique_ptr<FILE, internal::ScopedFILECloser>;

#endif  // FILE_H
