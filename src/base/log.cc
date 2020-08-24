#include "log.h"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <cstdio>
#endif
#include <cstdlib>

#include "vecmath.h"

namespace base {

// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
LogBase* LogBase::swallow_stream;

LogBase::LogBase(const char* file, int line) : file_(file), line_(line) {}

LogBase::~LogBase() = default;

template <>
LogBase& LogBase::operator<<<bool>(const bool& arg) {
  stream_ << (arg ? "true" : "false");
  return *this;
}

template <>
LogBase& LogBase::operator<<<Vector2>(const Vector2& arg) {
  stream_ << "(" << arg.x << ", " << arg.y << ")";
  return *this;
}

template <>
LogBase& LogBase::operator<<<Vector4>(const Vector4& arg) {
  stream_ << "(" << arg.x << ", " << arg.y << ", " << arg.z << ", " << arg.w
          << ")";
  return *this;
}

void LogBase::Flush() {
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

Log::Log(const char* file, int line) : LogBase(file, line) {}

Log::~Log() {
  Flush();
}

Check::Check(const char* file,
             int line,
             bool condition,
             bool debug,
             const char* expr)
    : LogBase(file, line), condition_(condition) {
  if (!condition_)
    *this << (debug ? "DCHECK: (" : "CHECK: (") << expr << ") ";
}

Check::~Check() {
  if (!condition_) {
    Flush();
    std::abort();
  }
}

NotReached::NotReached(const char* file, int line) : LogBase(file, line) {
  *this << "NOTREACHED ";
}

NotReached::~NotReached() {
  Flush();
  std::abort();
}

}  // namespace base
