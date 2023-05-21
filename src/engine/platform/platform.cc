#include "engine/platform/platform.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "engine/engine.h"
#include "engine/renderer/opengl/renderer_opengl.h"
#include "engine/renderer/vulkan/renderer_vulkan.h"

#if defined(__ANDROID__)
#include "engine/audio/audio_driver_oboe.h"
#elif defined(__linux__)
#include "engine/audio/audio_driver_alsa.h"
#endif

using namespace base;

namespace eng {

Platform::Platform() = default;

Platform::~Platform() = default;

void Platform::InitializeCommon() {
  LOG << "Initializing platform.";

  thread_pool_.Initialize();
  TaskRunner::CreateThreadLocalTaskRunner();

#if defined(__ANDROID__)
  audio_driver_ = std::make_unique<AudioDriverOboe>();
#elif defined(__linux__)
  audio_driver_ = std::make_unique<AudioDriverAlsa>();
#endif
  bool res = audio_driver_->Initialize();
  CHECK(res) << "Failed to initialize audio driver.";

  auto context = std::make_unique<VulkanContext>();
  if (context->Initialize()) {
    renderer_ = std::make_unique<RendererVulkan>(std::move(context));
  } else {
    LOG << "Failed to initialize Vulkan context. Fallback to OpenGL.";
    renderer_ = std::make_unique<RendererOpenGL>();
  }
}

void Platform::ShutdownCommon() {
  LOG << "Shutting down platform.";

  audio_driver_->Shutdown();
  renderer_->Shutdown();
}

void Platform::RunMainLoop() {
  engine_ =
      std::make_unique<Engine>(this, renderer_.get(), audio_driver_.get());
  bool res = engine_->Initialize();
  CHECK(res) << "Failed to initialize the engine.";

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
