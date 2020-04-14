#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

class Timer {
public:
  Timer();
  ~Timer() = default;

  void Reset();

  void Update();

  float GetSecondsPassed() const        { return secondsPassed; }
  float GetSecondsAccumulated() const   { return secondsAccumulated; }

private:
  float secondsPassed = 0.0f;
  float secondsAccumulated = 0.0f;

  timeval lastTime;
};

#endif // TIMER_H
