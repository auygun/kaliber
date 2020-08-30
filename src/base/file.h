#ifndef FILE_H
#define FILE_H

#include <cstdio>
#include <memory>

namespace base {

namespace internal {

struct ScopedFILECloser {
  inline void operator()(FILE* x) const {
    if (x)
      fclose(x);
  }
};

}  // namespace internal

// Automatically closes file.
using ScopedFILE = std::unique_ptr<FILE, internal::ScopedFILECloser>;

}  // namespace base

#endif  // FILE_H
