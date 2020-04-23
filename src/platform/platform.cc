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
  constexpr float time_step = 1.0f / 30.0f;
  constexpr float speed = 1.0f;

  Timer timer;
  float last_time = timer.GetSecondsAccumulated();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  bool should_draw = true;
  for (;;) {
    if (should_draw) {
      // LOG("Draw!!!\n");
      should_draw = false;
      engine::Engine::Get().Draw(frame_frac);

      Update();
      if (should_exit_) {
        engine::Engine::Get().Shutdown();
        return;
      }
    }

    timer.Update();
    float new_time = timer.GetSecondsAccumulated();
    float frame_time = (new_time - last_time) * speed;
    last_time = new_time;
    accumulator += frame_time;
    // LOG("accumulator: %f\n", accumulator);

    if (accumulator < time_step) {
      float sleep_time = time_step - accumulator;
      // LOG("sleep for: %f\n", sleep_time);
      std::this_thread::sleep_for(std::chrono::nanoseconds((int)(sleep_time * 1000000000.0f)));
      accumulator += sleep_time;
    }

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      engine::Engine::Get().Update(time_step);
      accumulator -= time_step;
      should_draw = true;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}
