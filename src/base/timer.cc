#include "timer.h"

namespace base {

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  gettimeofday(&last_time_, nullptr);

  seconds_passed_ = 0.0f;
  seconds_accumulated_ = 0.0f;
}

void Timer::Update() {
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);
  seconds_passed_ =
      (float)(currentTime.tv_sec - last_time_.tv_sec) +
      0.000001f * (float)(currentTime.tv_usec - last_time_.tv_usec);

  last_time_ = currentTime;

  seconds_accumulated_ += seconds_passed_;
}

}  // namespace base
