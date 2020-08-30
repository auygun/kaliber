#ifndef LOG_H
#define LOG_H

#include <sstream>

// Macros for logging that are active in both debug and release builds. The way
// to log things is to stream things to LOG.
// LOG_DIFF can be used to avoid spam and log only if the message differs.
// CHECK(condition) terminates the process if the condition is false.
// NOTREACHED annotates unreachable codepaths and terminates the process if
// reached.
#define LOG base::Log(__FILE__, __LINE__).base()
#define LOG_DIFF base::LogDiff(__FILE__, __LINE__).base()
#define CHECK(expr) \
  base::Check(__FILE__, __LINE__, static_cast<bool>(expr), false, #expr).base()
#define NOTREACHED base::NotReached(__FILE__, __LINE__).base()

// Macros for logging which are active only in debug builds.
#ifdef _DEBUG
#define DLOG base::Log(__FILE__, __LINE__).base()
#define DLOG_DIFF base::LogDiff(__FILE__, __LINE__).base()
#define DCHECK(expr) \
  base::Check(__FILE__, __LINE__, static_cast<bool>(expr), true, #expr).base()
#else
// "debug mode" logging is compiled away to nothing for release builds.
#define DLOG EAT_STREAM_PARAMETERS
#define DLOG_DIFF EAT_STREAM_PARAMETERS
#define DCHECK(expr) EAT_STREAM_PARAMETERS
#endif

// Adapted from Chromium's logging implementation.
// Avoid any pointless instructions to be emitted by the compiler.
#define EAT_STREAM_PARAMETERS \
  true ? (void)0 : base::LogBase::Voidify() & (*base::LogBase::swallow_stream)

namespace base {

struct Vector2;
struct Vector3;
struct Vector4;

class LogBase {
 public:
  class Voidify {
   public:
    Voidify() = default;
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(LogBase&) {}
  };

  LogBase& base() { return *this; }

  std::ostream& stream() { return stream_; }

  static LogBase* swallow_stream;

 protected:
  const char* file_;
  const int line_;
  std::ostringstream stream_;

  LogBase(const char* file, int line);
  ~LogBase();

  void Flush();
};

class Log : public LogBase {
 public:
  Log(const char* file, int line);
  ~Log();
};

class LogDiff : public LogBase {
 public:
  LogDiff(const char* file, int line);
  ~LogDiff();
};

class Check : public LogBase {
 public:
  Check(const char* file,
        int line,
        bool condition,
        bool debug,
        const char* expr);
  ~Check();

 private:
  bool condition_;
};

class NotReached : public LogBase {
 public:
  NotReached(const char* file, int line);
  ~NotReached();
};

template <typename T>
LogBase& operator<<(LogBase& out, const T& arg) {
  out.stream() << arg;
  return out;
}

// Explicit specialization for internal types.
template <>
LogBase& operator<<(LogBase& out, const base::Vector2& arg);
template <>
LogBase& operator<<(LogBase& out, const base::Vector3& arg);
template <>
LogBase& operator<<(LogBase& out, const base::Vector4& arg);

}  // namespace base

#endif  // LOG_H
