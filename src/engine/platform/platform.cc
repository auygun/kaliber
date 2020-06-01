#include "platform.h"

#include <thread>

#include "../../base/log.h"
#include "../engine.h"
#include "../renderer/renderer.h"

// Save battery on mobile devices.
#define USE_SLEEP

namespace eng {

Platform::InternalError Platform::internal_error;

void Platform::Shutdown() {
  LOG << "Shutting down platform.";
  renderer_->Shutdown();
}

void Platform::RunMainLoop() {
  engine_ = std::make_unique<Engine>(this, renderer_.get());
  if (!engine_->Initialize()) {
    LOG << "Failed to initialize the engine.";
    throw internal_error;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;
  constexpr float epsilon = 0.0001f;

  timer_.Reset();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;) {
    engine_->Draw(frame_frac);

    // Accumulate time.
#ifdef USE_SLEEP
    while (accumulator < time_step) {
      timer_.Update();
      accumulator += timer_.GetSecondsPassed();
      if (time_step - accumulator > epsilon) {
        float sleep_time = time_step - accumulator - epsilon;
        std::this_thread::sleep_for(
            std::chrono::microseconds((int)(sleep_time * 1000000.0f)));
      }
    };
#else
    timer_.Update();
    accumulator += timer_.GetSecondsPassed();
#endif  // USE_SLEEP

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      Update();
      if (should_exit_) {
        engine_->Shutdown();
        engine_.reset();
        return;
      }
      engine_->Update(time_step * speed);
      accumulator -= time_step;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}

}  // namespace eng
