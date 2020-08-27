#include "log.h"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <cstdio>
#endif
#include <cstdlib>
#include <mutex>
#include <unordered_map>

#include "vecmath.h"

namespace base {

// Adapted from Chromium's logging implementation.
// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
LogBase* LogBase::swallow_stream;

LogBase::LogBase(const char* file, int line) : file_(file), line_(line) {}

LogBase::~LogBase() = default;

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

LogDiff::LogDiff(const char* file, int line) : LogBase(file, line) {}

LogDiff::~LogDiff() {
  static std::unordered_map<std::string, std::string> log_map;
  static std::mutex lock;

  auto key = std::string(file_) + std::to_string(line_);
  bool flush = true;
  {
    std::lock_guard<std::mutex> scoped_lock(lock);
    auto it = log_map.find(key);
    if (it == log_map.end())
      log_map[key] = stream_.str();
    else if (it->second != stream_.str())
      it->second = stream_.str();
    else
      flush = false;
  }

  if (flush)
    Flush();
}

Check::Check(const char* file,
             int line,
             bool condition,
             bool debug,
             const char* expr)
    : LogBase(file, line), condition_(condition) {
  if (!condition_)
    base() << (debug ? "DCHECK: (" : "CHECK: (") << expr << ") ";
}

Check::~Check() {
  if (!condition_) {
    Flush();
    std::abort();
  }
}

NotReached::NotReached(const char* file, int line) : LogBase(file, line) {
  base() << "NOTREACHED ";
}

NotReached::~NotReached() {
  Flush();
  std::abort();
}

template <>
LogBase& operator<<(LogBase& out, const base::Vector2& arg) {
  out.stream() << "(" << arg.x << ", " << arg.y << ")";
  return out;
}

template <>
LogBase& operator<<(LogBase& out, const base::Vector3& arg) {
  out.stream() << "(" << arg.x << ", " << arg.y << ", " << arg.z << ")";
  return out;
}

template <>
LogBase& operator<<(LogBase& out, const base::Vector4& arg) {
  out.stream() << "(" << arg.x << ", " << arg.y << ", " << arg.z << ", "
               << arg.w << ")";
  return out;
}

}  // namespace base
