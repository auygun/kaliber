#include "engine/engine.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/asset/sound.h"
#include "engine/audio/audio_mixer.h"
#include "engine/fly_camera.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "engine/input_system.h"
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

  // Maximum time we allow the simulation to fall behind real-time.
  constexpr float max_accumulator = 0.25f;  // 250ms is a common safe value.

  for (;;) {
    platform_->Update();
    if (platform_->should_exit())
      return;

    if (!renderer_->IsInitialzed())
      continue;

    // Capture frame time once.
    float frame_delta = timer_.Delta();
    accumulator += frame_delta;

    // Safety clamp to prevent "spiral of death" on lag spikes.
    if (accumulator > max_accumulator)
      accumulator = max_accumulator;

    // Subdivide the frame time using fixed time steps.
    while (accumulator >= time_step_) {
      FixedUpdate(time_step_);
      accumulator -= time_step_;
    };

    // Variable update
    Update(frame_delta);

    // Calculate frame fraction from remainder of the frame time.
    float frame_frac = accumulator / time_step_;
    Draw(frame_frac);
  }
}

void Engine::Initialize() {
  LOG(0) << "Initializing engine.";

  thread_pool_.Initialize();

  input_system_.Init(world_.GetRegistry());

  imgui_backend_.Initialize(IsMobile(), GetRootPath());

  platform_->CreateMainWindow();

  CreateRendererInternal(RendererType::kVulkan);

  float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
  LOG(0) << "aspect_ratio: " << aspect_ratio;
  screen_size_ = {1.0f, aspect_ratio * 1.0f};

  world_.Create(renderer_.get());

  input_system_.Init(world_.GetRegistry());

  systems_.push_back(std::make_unique<FlyCamera>());
  systems_.back()->Init(world_);

  game_ = GameFactoryBase::CreateGame("");
  CHECK(game_) << "No game found to run.";
  CHECK(game_->Initialize()) << "Failed to initialize the game.";

  imgui_backend_.CreateRenderResources(renderer_.get());

  auto cam_entity = world_.CreateSceneNode("cam");
  auto& registry = world_.GetRegistry();
  registry.AddComponent(cam_entity, FlyCameraComponent{.speed = 4.0f});
  registry.AddComponent(cam_entity, PrimaryCameraTag{});
  registry.AddComponent(
      cam_entity,
      CameraComponent{.fov = 45.0f, .near_plane = 1.0f, .far_plane = 1000.0f});
}

void Engine::FixedUpdate(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;
}

void Engine::Update(float delta_time) {
  imgui_backend_.NewFrame(delta_time);
  auto [mouse_captured, keyboard_captured] =
      imgui_backend_.ProcessInput(platform_);

  input_system_.Update(mouse_captured, keyboard_captured);

  // Scene graph update for objects moved by physics
  world_.SceneGraphUpdate();

  TaskRunner::GetThreadLocalTaskRunner()->RunTasks<Consumer::Single>();

  for (auto& system : systems_)
    system->Update(world_, delta_time);

  game_->Update(delta_time);

  world_.Update(delta_time);

  // Scene graph update (Post-Logic finalize)
  world_.SceneGraphUpdate();

  fps_seconds_ += delta_time;
  if (fps_seconds_ >= 1) {
    fps_ = renderer_->GetAndResetFPS();
    fps_seconds_ = 0;
  }

  if (stats_visible_)
    ShowStats();
}

void Engine::Draw(float frame_frac) {
  renderer_->PrepareForDrawing();
  // game_->Render(frame_frac);
  world_.Render(frame_frac);
  imgui_backend_.Draw();
  renderer_->Present();
}

void Engine::CreateRenderer(RendererType type) {
  // Create a new renderer next cycle.
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::CreateRendererInternal, this, type));
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::ContextLost, this));
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
