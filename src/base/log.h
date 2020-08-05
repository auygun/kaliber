#ifndef LOG_H
#define LOG_H

#include <sstream>
#include "vecmath.h"

#define EAT_STREAM_PARAMETERS \
  true ? (void)0 : base::Log::Voidify() & (*base::Log::swallow_stream)

#define LOG base::Log(__FILE__, __LINE__)
#define CHECK(condition) \
  base::Check(__FILE__, __LINE__, condition, false, #condition)
#define NOTREACHED base::NotReached(__FILE__, __LINE__)

#ifdef _DEBUG
#define DLOG base::Log(__FILE__, __LINE__)
#define DCHECK(condition) \
  base::Check(__FILE__, __LINE__, condition, true, #condition)
#else
#define DLOG EAT_STREAM_PARAMETERS
#define DCHECK(condition) EAT_STREAM_PARAMETERS
#endif

namespace base {

class LogBase {
 public:
  class Voidify {
   public:
    Voidify() = default;
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(LogBase&) {}
  };

  LogBase(const char* file, int line);
  ~LogBase();

  template <typename T>
  LogBase& operator<<(const T& arg) {
    stream_ << arg;
    return *this;
  }

  LogBase& operator<<(const bool& arg);
  LogBase& operator<<(const Vector2& arg);
  LogBase& operator<<(const Vector4& arg);

  static LogBase* swallow_stream;

 protected:
  void Flush();

 private:
  const char* file_;
  const int line_;
  std::ostringstream stream_;
};

class Log : public LogBase {
 public:
  Log(const char* file, int line);
  ~Log();
};

class Check : public LogBase {
 public:
  Check(const char* file,
        int line,
        bool condition,
        bool debug,
        const char* condition_str);
  ~Check();

 private:
  bool condition_;
};

class NotReached : public LogBase {
 public:
  NotReached(const char* file, int line);
  ~NotReached();
};

}  // namespace base

#endif  // LOG_H
