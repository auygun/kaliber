#include "timer.h"

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  gettimeofday(&lastTime, nullptr);

  secondsPassed = 0.0f;
  secondsAccumulated = 0.0f;
}

void Timer::Update() {
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);
  secondsPassed =             (float)(currentTime.tv_sec - lastTime.tv_sec) +
                  0.000001f * (float)(currentTime.tv_usec - lastTime.tv_usec);

  lastTime = currentTime;

  secondsAccumulated += secondsPassed;
}
