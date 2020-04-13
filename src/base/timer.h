#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

namespace base {

class Timer {
 public:
  Timer();
  ~Timer() = default;

  void Reset();

  void Update();

  float GetSecondsPassed() const { return seconds_passed_; }
  float GetSecondsAccumulated() const { return seconds_accumulated_; }

 private:
  float seconds_passed_ = 0.0f;
  float seconds_accumulated_ = 0.0f;

  timeval last_time_;
};

}  // namespace base

#endif  // TIMER_H
