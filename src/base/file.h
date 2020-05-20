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

}  // namespace internal

namespace base {

// Automatically closes file.
using ScopedFILE = std::unique_ptr<FILE, internal::ScopedFILECloser>;

}  // namespace base

#endif  // FILE_H
