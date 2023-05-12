#include "engine/platform/platform.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "engine/audio/audio.h"
#include "engine/engine.h"
#include "engine/renderer/opengl/renderer_opengl.h"
#include "engine/renderer/vulkan/renderer_vulkan.h"

#define USE_VULKAN_RENDERER 1

using namespace base;

namespace eng {

Platform::InternalError Platform::internal_error;

Platform::Platform() = default;

Platform::~Platform() = default;

void Platform::InitializeCommon() {
  LOG << "Initializing platform.";

  thread_pool_.Initialize();
  TaskRunner::CreateThreadLocalTaskRunner();

  audio_ = std::make_unique<Audio>();
  if (!audio_->Initialize()) {
    LOG << "Failed to initialize audio system.";
    throw internal_error;
  }

#if (USE_VULKAN_RENDERER == 1)
  renderer_ = std::make_unique<RendererVulkan>();
#else
  renderer_ = std::make_unique<RendererOpenGL>();
#endif
}

void Platform::ShutdownCommon() {
  LOG << "Shutting down platform.";

  audio_->Shutdown();
  renderer_->Shutdown();
}

void Platform::RunMainLoop() {
  engine_ = std::make_unique<Engine>(this, renderer_.get(), audio_.get());
  if (!engine_->Initialize()) {
    LOG << "Failed to initialize the engine.";
    throw internal_error;
  }

  // Use fixed time steps.
  float time_step = engine_->time_step();

  timer_.Reset();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;) {
    engine_->Draw(frame_frac);

    // Accumulate time.
    timer_.Update();
    accumulator += timer_.GetSecondsPassed();

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      TaskRunner::GetThreadLocalTaskRunner()->SingleConsumerRun();

      Update();
      engine_->Update(time_step);

      if (should_exit_) {
        thread_pool_.Shutdown();
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
