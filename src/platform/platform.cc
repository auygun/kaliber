#include "platform.h"
#include "../base/log.h"
#include "../base/timer.h"
#include "../engine/engine.h"
#include <math.h>
#include <thread>

Platform::InternalError Platform::internal_error;

Platform& Platform::Get() {
  static Platform platform;
  return platform;
}

void Platform::RunMainLoop() {
  if (!engine::Engine::Get().Init()) {
    printf("Failed to initialize the engine.\n");
    throw internal_error;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;
  constexpr float epsilon = 0.000001f;

  Timer timer;
  float last_time = timer.GetSecondsAccumulated();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;) {
    engine::Engine::Get().Draw(frame_frac);

    Update();
    if (should_exit_) {
      engine::Engine::Get().Shutdown();
      return;
    }

    // Accumulate time.
    while (accumulator < time_step) {
      timer.Update();
      float new_time = timer.GetSecondsAccumulated();
      accumulator += new_time - last_time;
      last_time = new_time;
      if (time_step - accumulator > epsilon) {
        float sleep_time = time_step - accumulator - epsilon;
        std::this_thread::sleep_for(std::chrono::microseconds((int)(sleep_time * 1000000.0f)));
        accumulator += sleep_time;
      }
    };

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      engine::Engine::Get().Update(time_step * speed);
      accumulator -= time_step;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}
