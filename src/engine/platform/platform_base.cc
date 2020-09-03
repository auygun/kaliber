#include "platform.h"

#include <thread>

#include "../../base/log.h"
#include "../../base/task_runner.h"
#include "../audio/audio.h"
#include "../engine.h"
#include "../renderer/renderer.h"

// Save battery on mobile devices.
#define USE_SLEEP

using namespace base;

namespace eng {

PlatformBase::InternalError PlatformBase::internal_error;

PlatformBase::PlatformBase() = default;

PlatformBase::~PlatformBase() = default;

void PlatformBase::Initialize() {
  LOG << "Initializing platform.";

  worker_.Initialize();
  TaskRunner::CreateThreadLocalTaskRunner();

  audio_ = std::make_unique<Audio>();
  if (!audio_->Initialize()) {
    LOG << "Failed to initialize audio system.";
    throw internal_error;
  }

  renderer_ = std::make_unique<Renderer>();
}

void PlatformBase::Shutdown() {
  LOG << "Shutting down platform.";

  audio_->Shutdown();
  renderer_->Shutdown();
}

void PlatformBase::RunMainLoop() {
  engine_ = std::make_unique<Engine>(static_cast<Platform*>(this),
                                     renderer_.get(), audio_.get());
  if (!engine_->Initialize()) {
    LOG << "Failed to initialize the engine.";
    throw internal_error;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;

#ifdef USE_SLEEP
  constexpr float epsilon = 0.0001f;
#endif  // USE_SLEEP

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
      TaskRunner::GetThreadLocalTaskRunner()->SingleConsumerRun();

      static_cast<Platform*>(this)->Update();
      engine_->Update(time_step);

      if (should_exit_) {
        worker_.Shutdown();
        engine_->Shutdown();
        engine_.reset();
        return;
      }
      accumulator -= time_step;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}

}  // namespace eng
