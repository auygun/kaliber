#include "engine/engine.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/asset/sound.h"
#include "engine/audio/audio_mixer.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "engine/input_event.h"
#include "engine/platform/platform.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui.h"
#include "third_party/texture_compressor/texture_compressor.h"

using namespace base;

namespace eng {

extern void KaliberMain(Platform* platform) {
  TaskRunner::CreateThreadLocalTaskRunner();
  Engine(platform).Run();
}

Engine* Engine::singleton = nullptr;

Engine::Engine(Platform* platform)
    : platform_(platform), audio_mixer_{std::make_unique<AudioMixer>()} {
  DCHECK(!singleton);
  singleton = this;

  platform_->SetObserver(this);
}

Engine::~Engine() noexcept {
  LOG(0) << "Shutting down engine.";

  thread_pool_.CancelTasks();
  thread_pool_.Shutdown();

  imgui_backend_.Shutdown();
  game_.reset();
  renderer_.reset();
  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

void Engine::Run() {
  Initialize();

  timer_ = DeltaTimer();
  float accumulator = 0.0f;

  for (;;) {
    platform_->Update();
    if (platform_->should_exit())
      return;

    if (!renderer_->IsInitialzed())
      continue;

    // Accumulate time.
    accumulator += timer_.Delta();

    // Subdivide the frame time using fixed time steps.
    while (accumulator >= time_step_) {
      Update(time_step_);
      accumulator -= time_step_;
    };

    TaskRunner::GetThreadLocalTaskRunner()->RunTasks<Consumer::Single>();

    // Calculate frame fraction from remainder of the frame time.
    float frame_frac = accumulator / time_step_;
    Draw(frame_frac);
  }
}

void Engine::Initialize() {
  LOG(0) << "Initializing engine.";

  thread_pool_.Initialize();

  imgui_backend_.Initialize(IsMobile(), GetRootPath());

  platform_->CreateMainWindow();

  CreateRendererInternal(RendererType::kVulkan);

  float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
  LOG(0) << "aspect_ratio: " << aspect_ratio;
  screen_size_ = {1.0f, aspect_ratio * 1.0f};

  game_ = GameFactoryBase::CreateGame("");
  CHECK(game_) << "No game found to run.";
  CHECK(game_->Initialize()) << "Failed to initialize the game.";

  imgui_backend_.CreateRenderResources(renderer_.get());
  imgui_backend_.NewFrame(0);
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;

  imgui_backend_.NewFrame(delta_time);

  game_->Update(delta_time);

  fps_seconds_ += delta_time;
  if (fps_seconds_ >= 1) {
    fps_ = renderer_->GetAndResetFPS();
    fps_seconds_ = 0;
  }

  if (stats_visible_)
    ShowStats();

  imgui_backend_.EndFrame();
}

void Engine::Draw(float frame_frac) {
  renderer_->PrepareForDrawing();
  game_->Render(frame_frac);
  imgui_backend_.Draw();
  renderer_->Present();
}

void Engine::CreateRenderer(RendererType type) {
  // Create a new renderer next cycle.
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::CreateRendererInternal, this, type));
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::ContextLost, this));
  input_queue_.clear();
}

RendererType Engine::GetRendererType() {
  if (renderer_)
    return renderer_->GetRendererType();
  return RendererType::kUnknown;
}

void Engine::Exit() {
  platform_->Exit();
}

Vector2f Engine::ToViewportScale(const Vector2f& vec) {
  return GetViewportSize() * vec /
         Vector2f((float)GetScreenWidth(), (float)GetScreenHeight());
}

Vector2f Engine::ToViewportPosition(const Vector2f& vec) {
  return ToViewportScale(vec) - GetViewportSize() / 2.0f;
}

std::unique_ptr<InputEvent> Engine::GetNextInputEvent() {
  std::unique_ptr<InputEvent> event;
  if (!input_queue_.empty()) {
    event.swap(input_queue_.front());
    input_queue_.pop_front();
  }
  return event;
}

void Engine::Vibrate(int duration) {
  if (vibration_enabled_)
    platform_->Vibrate(duration);
}

void Engine::ShowInterstitialAd() {
  platform_->ShowInterstitialAd();
}

void Engine::ShareFile(const std::string& file_name) {
  platform_->ShareFile(file_name);
}

void Engine::SetKeepScreenOn(bool keep_screen_on) {
  platform_->SetKeepScreenOn(keep_screen_on);
}

void Engine::SetEnableAudio(bool enable) {
  audio_mixer_->SetEnableAudio(enable);
}

TextureCompressor* Engine::GetTextureCompressor(bool opacity) {
  return opacity ? tex_comp_alpha_.get() : tex_comp_opaque_.get();
}

int Engine::GetScreenWidth() const {
  return renderer_->GetScreenWidth();
}

int Engine::GetScreenHeight() const {
  return renderer_->GetScreenHeight();
}

const std::string& Engine::GetRootPath() const {
  return platform_->GetRootPath();
}

const std::string& Engine::GetDataPath() const {
  return platform_->GetDataPath();
}

const std::string& Engine::GetSharedDataPath() const {
  return platform_->GetSharedDataPath();
}

size_t Engine::GetAudioHardwareSampleRate() {
  return audio_mixer_->GetHardwareSampleRate();
}

bool Engine::IsMobile() const {
  return platform_->mobile_device();
}

void Engine::OnWindowCreated() {
  if (renderer_)
    renderer_->Initialize(platform_);
}

void Engine::OnWindowDestroyed() {
  renderer_->Shutdown();
}

void Engine::OnWindowResized(int width, int height) {
  if (renderer_ && (width != renderer_->GetScreenWidth() ||
                    height != renderer_->GetScreenHeight())) {
    renderer_->OnWindowResized(width, height);
    float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    LOG(0) << "aspect_ratio: " << aspect_ratio;
    screen_size_ = {1.0f, aspect_ratio * 1.0f};
    game_->OnWindowResized(width, height);
  }
}

void Engine::LostFocus() {
  audio_mixer_->Suspend();

  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus(bool from_interstitial_ad) {
  timer_ = DeltaTimer();
  audio_mixer_->Resume();

  if (game_)
    game_->GainedFocus(from_interstitial_ad);
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  event = imgui_backend_.OnInputEvent(std::move(event));
  if (!event)
    return;

  // event->SetVector(ToViewportPosition(event->GetVector()) * Vector2f(1, -1));

  switch (event->GetType()) {
    case InputEvent::kKeyPress:
      if (event->GetKeyPress() == 's') {
        stats_visible_ = !stats_visible_;
        // Consume event.
        return;
      }
      break;
    default:
      break;
  }

  input_queue_.push_back(std::move(event));
}

void Engine::CreateRendererInternal(RendererType type) {
  if (renderer_ && renderer_->GetRendererType() == type)
    return;

  renderer_ = Renderer::Create(type, std::bind(&Engine::ContextLost, this));
  bool result = renderer_->Initialize(platform_);
  CHECK(result) << "Failed to initialize " << renderer_->GetDebugName()
                << " renderer.";

  CreateTextureCompressors();
}

void Engine::CreateTextureCompressors() {
  tex_comp_alpha_.reset();
  tex_comp_opaque_.reset();

  if (renderer_->SupportsDXT5()) {
    tex_comp_alpha_ = TextureCompressor::Create(TextureCompressor::kFormatDXT5);
  } else if (renderer_->SupportsATC()) {
    tex_comp_alpha_ =
        TextureCompressor::Create(TextureCompressor::kFormatATCIA);
  }

  if (renderer_->SupportsDXT1()) {
    tex_comp_opaque_ =
        TextureCompressor::Create(TextureCompressor::kFormatDXT1);
  } else if (renderer_->SupportsATC()) {
    tex_comp_opaque_ = TextureCompressor::Create(TextureCompressor::kFormatATC);
  } else if (renderer_->SupportsETC1()) {
    tex_comp_opaque_ =
        TextureCompressor::Create(TextureCompressor::kFormatETC1);
  }
}

void Engine::ContextLost() {
  input_queue_.clear();

  if (game_)
    game_->ContextLost();
}

void Engine::ShowStats() {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Stats", nullptr, window_flags);
  ImGui::Text("%s", renderer_->GetDebugName());
  ImGui::Text("%d fps", fps_);
  ImGui::End();
}

}  // namespace eng
