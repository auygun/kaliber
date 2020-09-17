#ifndef LOG_H
#define LOG_H

#include <sstream>

// Adapted from Chromium's logging implementation.

// Macros for logging that are active in both debug and release builds. The way
// to log things is to stream things to LOG.
// LOG_IF can be used for conditional logging.
// CHECK(condition) terminates the process if the condition is false.
// NOTREACHED annotates unreachable codepaths and terminates the process if
// reached.
#define LOG base::LogMessage(__FILE__, __LINE__).stream()
#define LOG_IF(condition) \
  LAZY_STREAM(condition, base::LogMessage(__FILE__, __LINE__).stream())
#define CHECK(condition) \
  LAZY_STREAM(           \
      !(condition),      \
      base::LogAbort::Check(__FILE__, __LINE__, #condition).GetLog().stream())

#define NOTREACHED \
  base::LogAbort::NotReached(__FILE__, __LINE__).GetLog().stream()

// Macros for logging which are active only in debug builds.
#ifdef _DEBUG
#define DLOG base::LogMessage(__FILE__, __LINE__).stream()
#define DLOG_IF(condition) \
  LAZY_STREAM(condition, base::LogMessage(__FILE__, __LINE__).stream())
#define DCHECK(condition)                                            \
  LAZY_STREAM(!(condition),                                          \
              base::LogAbort::DCheck(__FILE__, __LINE__, #condition) \
                  .GetLog()                                          \
                  .stream())
#else
// "debug mode" logging is compiled away to nothing for release builds.
#define DLOG EAT_STREAM_PARAMETERS
#define DLOG_IF(condition) EAT_STREAM_PARAMETERS
#define DCHECK(condition) EAT_STREAM_PARAMETERS
#endif

// Helper macro which avoids evaluating the arguments to a stream if
// the condition doesn't hold.
#define LAZY_STREAM(condition, stream) \
  !(condition) ? (void)0 : base::LogMessage::Voidify() & (stream)

// Avoid any pointless instructions to be emitted by the compiler.
#define EAT_STREAM_PARAMETERS \
  LAZY_STREAM(false, *base::LogMessage::swallow_stream)

namespace base {

class LogMessage {
 public:
  class Voidify {
   public:
    Voidify() = default;

    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(std::ostream&) {}
  };

  LogMessage(const char* file, int line);
  ~LogMessage();

  LogMessage& base() { return *this; }

  std::ostream& stream() { return stream_; }

  static std::ostream* swallow_stream;

 protected:
  const char* file_;
  const int line_;
  std::ostringstream stream_;
};

class LogAbort {
 public:
  ~LogAbort();

  static LogAbort Check(const char* file, int line, const char* expr);
  static LogAbort DCheck(const char* file, int line, const char* expr);
  static LogAbort NotReached(const char* file, int line);

  LogMessage& GetLog() { return *log_; }

 private:
  LogMessage* log_;

  LogAbort(LogMessage* log);
};

}  // namespace base

#endif  // LOG_H
