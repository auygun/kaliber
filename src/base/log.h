#ifndef LOG_H
#define LOG_H

#include "vecmath.h"
#include <sstream>

#define EAT_STREAM_PARAMETERS \
  true ? (void)0              \
       : Log::Voidify() & (*Log::swallow_stream)

#define LOG Log(__FILE__, __LINE__)

#ifdef _DEBUG
#define DLOG Log(__FILE__, __LINE__)
#else
#define DLOG EAT_STREAM_PARAMETERS
#endif

class Log {
 public:
  class Voidify {
   public:
    Voidify() = default;
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(Log&) { }
  };

  Log(const char* file, int line);
  ~Log();

  template<typename T>
  Log& operator<<(const T& arg) {
    stream_ << arg;
    return *this;
  }

  template<>
  Log& operator<<<Vector2>(const Vector2& arg) {
    stream_ << "(" << arg.x << ", " << arg.y << ")";
    return *this;
  }

  static Log* swallow_stream;

 private:
  const char* file_;
  const int line_;
  std::ostringstream stream_;
};

#endif  // LOG_H
