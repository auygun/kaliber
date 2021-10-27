#include "base/timer.h"

#include <thread>

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

void Timer::Sleep(float duration) {
  Timer timer;
  float accumulator = 0.0;
  constexpr float epsilon = 0.0001f;

  while (accumulator < duration) {
    timer.Update();
    accumulator += timer.GetSecondsPassed();
    if (duration - accumulator > epsilon) {
      float sleep_time = duration - accumulator - epsilon;
      std::this_thread::sleep_for(
          std::chrono::microseconds((int)(sleep_time * 1000000.0f)));
    }
  };
}

}  // namespace base
