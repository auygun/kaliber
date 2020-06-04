#ifndef LOG_H
#define LOG_H

#include <sstream>
#include "vecmath.h"

#define EAT_STREAM_PARAMETERS \
  true ? (void)0 : base::Log::Voidify() & (*base::Log::swallow_stream)

#define LOG base::Log(__FILE__, __LINE__)

#ifdef _DEBUG
#define DLOG base::Log(__FILE__, __LINE__)
#else
#define DLOG EAT_STREAM_PARAMETERS
#endif

namespace base {

class Log {
 public:
  class Voidify {
   public:
    Voidify() = default;
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(Log&) {}
  };

  Log(const char* file, int line);
  ~Log();

  template <typename T>
  Log& operator<<(const T& arg) {
    stream_ << arg;
    return *this;
  }

  static Log* swallow_stream;

 private:
  const char* file_;
  const int line_;
  std::ostringstream stream_;
};

}  // namespace base

#endif  // LOG_H
