#ifndef LOG_H
#define LOG_H

#include <sstream>

#define EAT_STREAM_PARAMETERS \
  true ? (void)0 : base::Log::Voidify() & (*base::Log::swallow_stream)

#define LOG base::Log(__FILE__, __LINE__)
#define CHECK(expr) \
  base::Check(__FILE__, __LINE__, static_cast<bool>(expr), false, #expr)
#define NOTREACHED base::NotReached(__FILE__, __LINE__)

#ifdef _DEBUG
#define DLOG base::Log(__FILE__, __LINE__)
#define DLOG_ONCE base::LogOnce(__FILE__, __LINE__)
#define DCHECK(expr) \
  base::Check(__FILE__, __LINE__, static_cast<bool>(expr), true, #expr)
#else
#define DLOG EAT_STREAM_PARAMETERS
#define DLOG_ONCE EAT_STREAM_PARAMETERS
#define DCHECK(expr) EAT_STREAM_PARAMETERS
#endif

namespace base {

struct Vector2;
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

  template <typename T>
  LogBase& operator<<(const T& arg) {
    stream_ << arg;
    return *this;
  }

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

#ifdef _DEBUG

class LogOnce : public LogBase {
 public:
  LogOnce(const char* file, int line);
  ~LogOnce();
};

#endif

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

}  // namespace base

#endif  // LOG_H
