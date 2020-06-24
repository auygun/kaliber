#include "log.h"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <cstdio>
#endif

namespace base {

// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
Log* Log::swallow_stream;

Log::Log(const char* file, int line) : file_(file), line_(line) {}

Log::~Log() {
  stream_ << std::endl;
  std::string text(stream_.str());
  std::string filename(file_);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != std::string::npos)
    filename = filename.substr(last_slash_pos + 1);
#if defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_ERROR, "kaliber", "[%s:%d] %s",
                      filename.c_str(), line_, text.c_str());
#else
  printf("[%s:%d] %s", filename.c_str(), line_, text.c_str());
#endif
}

template <>
Log& Log::operator<<<bool>(const bool& arg) {
  stream_ << (arg ? "true" : "false");
  return *this;
}

template <>
Log& Log::operator<<<Vector2>(const Vector2& arg) {
  stream_ << "(" << arg.x << ", " << arg.y << ")";
  return *this;
}

template <>
Log& Log::operator<<<Vector4>(const Vector4& arg) {
  stream_ << "(" << arg.x << ", " << arg.y << ", " << arg.z << ", " << arg.w
          << ")";
  return *this;
}

}  // namespace base
