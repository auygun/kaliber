#ifndef CLOSURE_H
#define CLOSURE_H

#include <functional>
#include <string>
#include <tuple>

#ifdef _DEBUG

#define HERE std::make_tuple(__func__, __FILE__, __LINE__)

// Helper for logging location info, e.g. LOG << LOCATION(from)
#define LOCATION(from)                                                 \
  std::get<0>(from) << "() [" << [](const char* path) -> std::string { \
    std::string file_name(path);                                       \
    size_t last_slash_pos = file_name.find_last_of("\\/");             \
    if (last_slash_pos != std::string::npos)                           \
      file_name = file_name.substr(last_slash_pos + 1);                \
    return file_name;                                                  \
  }(std::get<1>(from)) << ":"                                          \
                       << std::get<2>(from) << "]"

#else

#define HERE nullptr
#define LOCATION(from) ""

#endif

namespace base {

using Closure = std::function<void()>;

#ifdef _DEBUG

// Provides location info (function name, file name and line number) where of a
// Closure was constructed.
using Location = std::tuple<const char*, const char*, int>;

#else

using Location = std::nullptr_t;

#endif

}  // namespace base

#endif  // CLOSURE_H
