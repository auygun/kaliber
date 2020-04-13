#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

class Timer {
public:
  bool Init();
  void Update();

  float GetSecondsPassed() const        { return secondsPassed; }
  float GetSecondsAccumulated() const   { return secondsAccumulated; }

private:
  float secondsPassed,
        secondsAccumulated;

  timeval lastTime;
};

#endif // TIMER_H
