#include "platform.h"
#include "../base/log.h"
#include "../base/timer.h"
#include "../engine/engine.h"
#include <math.h>
#include <thread>

#define USE_SLEEP

Platform::InternalError Platform::internal_error;

Platform& Platform::Get() {
  static Platform platform;
  return platform;
}

void Platform::RunMainLoop() {
  if (!eng::Engine::Get().Init()) {
    printf("Failed to initialize the engine.\n");
    throw internal_error;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;
  constexpr float epsilon = 0.0001f;

  Timer timer;
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;) {
    eng::Engine::Get().Draw(frame_frac);

#ifdef USE_SLEEP
    // Accumulate time.
    while (accumulator < time_step) {
      timer.Update();
      accumulator += timer.GetSecondsPassed();
      if (time_step - accumulator > epsilon) {
        float sleep_time = time_step - accumulator - epsilon;
        std::this_thread::sleep_for(std::chrono::microseconds((int)(sleep_time * 1000000.0f)));
      }
    };
#else
    timer.Update();
    accumulator += timer.GetSecondsPassed();
#endif // USE_SLEEP

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      Update();
      if (should_exit_) {
        eng::Engine::Get().Shutdown();
        return;
      }
      eng::Engine::Get().Update(time_step * speed);
      accumulator -= time_step;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}
