#include "engine/engine.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "base/timer.h"
#include "engine/animator.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/asset/sound.h"
#include "engine/audio/audio_mixer.h"
#include "engine/drawable.h"
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
  textures_.clear();
  shaders_.clear();
  quad_.Destroy();
  pass_through_shader_.Destroy();
  solid_shader_.Destroy();
  renderer_.reset();
  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

void Engine::Run() {
  Initialize();

  DeltaTimer timer;
  float accumulator = 0.0f;

  for (;;) {
    platform_->Update();
    if (platform_->should_exit())
      return;

    if (!renderer_->IsInitialzed())
      continue;

    // Accumulate time.
    accumulator += timer.Delta();

    // Subdivide the frame time using fixed time steps.
    while (accumulator >= time_step_) {
      Update(time_step_);
      accumulator -= time_step_;
    };

    TaskRunner::GetThreadLocalTaskRunner()->RunTasks();

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

  CreateProjectionMatrix();

  system_font_ = std::make_unique<Font>();
  system_font_->Load("engine/RobotoMono-Regular.ttf");

  engine_state_ = State::kPreInitializing;
  game_ = GameFactoryBase::CreateGame("");
  CHECK(game_) << "No game found to run.";
  CHECK(game_->PreInitialize()) << "Failed to pre-initialize the game.";

  // Create resources and let the game finalize initialization.
  CreateRenderResources();
  WaitForAsyncWork();

  engine_state_ = State::kInitializing;
  CHECK(game_->Initialize()) << "Failed to initialize the game.";
  engine_state_ = State::kInitialized;

  imgui_backend_.NewFrame(0);
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;

  imgui_backend_.NewFrame(delta_time);

  for (auto d : animators_)
    d->Update(delta_time);

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
  for (auto d : animators_)
    d->Evaluate(time_step_ * frame_frac);

  drawables_.sort(
      [](auto& a, auto& b) { return a->GetZOrder() < b->GetZOrder(); });

  renderer_->PrepareForDrawing();
  for (auto d : drawables_) {
    if (d->IsVisible())
      d->Draw(frame_frac);
  }
  imgui_backend_.Draw();
  renderer_->Present();
}

void Engine::AddDrawable(Drawable* drawable) {
  DCHECK(std::find(drawables_.begin(), drawables_.end(), drawable) ==
         drawables_.end());
  drawables_.push_back(drawable);
}

void Engine::RemoveDrawable(Drawable* drawable) {
  auto it = std::find(drawables_.begin(), drawables_.end(), drawable);
  if (it != drawables_.end()) {
    drawables_.erase(it);
    return;
  }
}

void Engine::AddAnimator(Animator* animator) {
  DCHECK(std::find(animators_.begin(), animators_.end(), animator) ==
         animators_.end());
  animators_.push_back(animator);
}

void Engine::RemoveAnimator(Animator* animator) {
  auto it = std::find(animators_.begin(), animators_.end(), animator);
  if (it != animators_.end()) {
    animators_.erase(it);
    return;
  }
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

void Engine::SetImageSource(const std::string& asset_name,
                            const std::string& file_name,
                            bool persistent) {
  SetImageSource(
      asset_name,
      [file_name]() -> std::unique_ptr<Image> {
        auto image = std::make_unique<Image>();
        if (!image->Load(file_name))
          return nullptr;
        image->Compress();
        return image;
      },
      persistent);
}

void Engine::SetImageSource(const std::string& asset_name,
                            CreateImageCB create_image,
                            bool persistent) {
  if (textures_.contains(asset_name)) {
    DLOG(0) << "Texture already exists: " << asset_name;
    return;
  }

  std::shared_ptr<Texture> texture;
  if (persistent)
    texture = std::make_shared<Texture>(renderer_.get());
  textures_[asset_name] = {texture, texture, create_image};
}

void Engine::RefreshImage(const std::string& asset_name) {
  DCHECK(engine_state_ != State::kPreInitializing);

  auto it = textures_.find(asset_name);
  if (it == textures_.end()) {
    DLOG(0) << "Texture not found: " << asset_name;
    return;
  }

  std::shared_ptr<Texture> texture = it->second.texture.lock();
  if (texture) {
    auto image = it->second.create_image();
    if (image)
      texture->Update(std::move(image));
  }
}

std::shared_ptr<Texture> Engine::AcquireTexture(const std::string& asset_name) {
  DCHECK(engine_state_ != State::kPreInitializing);

  auto it = textures_.find(asset_name);
  if (it == textures_.end()) {
    DLOG(0) << "Texture not found: " << asset_name;
    return nullptr;
  }

  std::shared_ptr<Texture> texture = it->second.texture.lock();
  if (!texture) {
    DCHECK(!it->second.persistent_ptr);
    texture = std::make_shared<Texture>(renderer_.get());
    it->second.texture = texture;
  }

  if (!texture->IsValid()) {
    auto image = it->second.create_image();
    if (image)
      texture->Update(std::move(image));
  }
  return texture;
}

void Engine::SetShaderSource(const std::string& asset_name,
                             const std::string& file_name) {
  if (shaders_.contains(asset_name)) {
    DLOG(0) << "Shader already exists: " << asset_name;
    return;
  }

  shaders_[asset_name] = {{}, file_name};
}

std::shared_ptr<Shader> Engine::GetShader(const std::string& asset_name) {
  DCHECK(engine_state_ != State::kPreInitializing);

  auto it = shaders_.find(asset_name);
  if (it == shaders_.end()) {
    DLOG(0) << "Shader not found: " << asset_name;
    return nullptr;
  }

  std::shared_ptr<Shader> shader = it->second.shader.lock();
  if (!shader) {
    shader = std::make_shared<Shader>(renderer_.get());
    it->second.shader = shader;
  }

  if (!shader->IsValid()) {
    auto source = std::make_unique<ShaderSource>();
    if (source->Load(it->second.file_name))
      shader->Create(std::move(source), quad_.vertex_description(),
                     quad_.primitive(), false);
  }

  return shader;
}

void Engine::SetAudioSource(const std::string& asset_name,
                            const std::string& file_name,
                            bool stream) {
  if (audio_buses_.contains(asset_name)) {
    DLOG(0) << "AudioBus already exists: " << asset_name;
    return;
  }

  auto sound = std::make_shared<Sound>();
  audio_buses_[asset_name] = sound;

  if (engine_state_ == State::kPreInitializing) {
    ++async_work_count_;
    thread_pool_.PostTaskAndReply(
        HERE, std::bind(&Sound::Load, sound, file_name, stream),
        [&]() -> void { --async_work_count_; });
  } else {
    sound->Load(file_name, stream);
  }
}

std::shared_ptr<AudioBus> Engine::GetAudioBus(const std::string& asset_name) {
  DCHECK(engine_state_ != State::kPreInitializing);

  auto it = audio_buses_.find(asset_name);
  if (it == audio_buses_.end()) {
    DLOG(0) << "AudioBus not found: " << asset_name;
    return nullptr;
  }

  return it->second;
}

std::unique_ptr<InputEvent> Engine::GetNextInputEvent() {
  std::unique_ptr<InputEvent> event;

  if (replaying_) {
    if (replay_index_ < replay_data_.root()["input"].size()) {
      auto data = replay_data_.root()["input"][replay_index_];
      if (data["tick"].asUInt64() == tick_) {
        event = std::make_unique<InputEvent>(
            (InputEvent::Type)data["input_type"].asInt(),
            (size_t)data["pointer_id"].asUInt(),
            Vector2f(data["pos_x"].asFloat(), data["pos_y"].asFloat()));
        ++replay_index_;
      }
      return event;
    }
    replaying_ = false;
    replay_data_.root().clear();
  }

  if (!input_queue_.empty()) {
    event.swap(input_queue_.front());
    input_queue_.pop_front();

    if (recording_) {
      Json::Value data;
      data["tick"] = tick_;
      data["input_type"] = event->GetType();
      data["pointer_id"] = event->GetPointerId();
      data["pos_x"] = event->GetVector().x;
      data["pos_y"] = event->GetVector().y;
      replay_data_.root()["input"].append(data);
    }
  }

  return event;
}

void Engine::StartRecording(const Json::Value& payload) {
  if (!replaying_ && !recording_) {
    recording_ = true;
    random_ = Randomf();
    replay_data_.root()["seed"] = random_.seed();
    replay_data_.root()["payload"] = payload;
    tick_ = 0;
  }
}

void Engine::EndRecording(const std::string file_name) {
  if (recording_) {
    DCHECK(!replaying_);

    recording_ = false;
    replay_data_.SaveAs(file_name, PersistentData::kShared);
    replay_data_.root().clear();
  }
}

bool Engine::Replay(const std::string file_name, Json::Value& payload) {
  if (!replaying_ && !recording_ &&
      replay_data_.Load(file_name, PersistentData::kShared)) {
    replaying_ = true;
    random_ = Randomf(replay_data_.root()["seed"].asUInt());
    payload = replay_data_.root()["payload"];
    tick_ = 0;
    replay_index_ = 0;
  }

  return replaying_;
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

std::unique_ptr<Image> Engine::Print(const std::string& text,
                                     Vector4f bg_color) {
  int width, height;
  system_font_->CalculateBoundingBox(text.c_str(), width, height);

  auto image = std::make_unique<Image>();
  image->Create(width, height);
  image->Clear(bg_color);
  system_font_->Print(0, 0, text.c_str(), image->GetBuffer(),
                      image->GetWidth());
  return image;
}

int Engine::GetScreenWidth() const {
  return renderer_->GetScreenWidth();
}

int Engine::GetScreenHeight() const {
  return renderer_->GetScreenHeight();
}

float Engine::GetImageScaleFactor() const {
  return static_cast<float>(renderer_->GetScreenWidth()) / 514.286f;
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
    CreateProjectionMatrix();
  }
}

void Engine::LostFocus() {
  audio_mixer_->Suspend();

  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus(bool from_interstitial_ad) {
  audio_mixer_->Resume();

  if (game_)
    game_->GainedFocus(from_interstitial_ad);
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  event = imgui_backend_.OnInputEvent(std::move(event));
  if (!event)
    return;

  if (replaying_)
    return;

  event->SetVector(ToViewportPosition(event->GetVector()) * Vector2f(1, -1));

  switch (event->GetType()) {
    case InputEvent::kDragEnd:
      if (((GetViewportSize() / 2) * 0.9f - event->GetVector()).Length() <=
          0.25f) {
        stats_visible_ = !stats_visible_;
        // TODO: Enqueue DragCancel so we can consume this event.
      }
      break;
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
  if (!result && type == RendererType::kVulkan) {
    LOG(0) << "Failed to initialize " << renderer_->GetDebugName()
           << " renderer.";
    LOG(0) << "Fallback to OpenGL renderer.";
    CreateRendererInternal(RendererType::kOpenGL);
    return;
  }
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

void Engine::CreateProjectionMatrix() {
  float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
  LOG(0) << "aspect_ratio: " << aspect_ratio;
  screen_size_ = {1.0f, aspect_ratio * 1.0f};
  projection_.CreateOrthographicProjection(-0.5f, 0.5f, -aspect_ratio * 0.5f,
                                           aspect_ratio * 0.5f);
}

void Engine::ContextLost() {
  CreateRenderResources();
  WaitForAsyncWork();
  input_queue_.clear();

  if (game_)
    game_->ContextLost();
}

void Engine::CreateRenderResources() {
  quad_.SetRenderer(renderer_.get());
  pass_through_shader_.SetRenderer(renderer_.get());
  solid_shader_.SetRenderer(renderer_.get());

  // This creates a normalized unit sized quad.
  static const char vertex_description[] = "p2f;t2f";
  static const float vertices[] = {
      -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  0.0f, 0.0f, 0.5f, 0.5f,  1.0f, 0.0f,
  };

  // Create the quad geometry we can reuse for all sprites.
  auto quad_mesh = std::make_unique<Mesh>();
  quad_mesh->Create(kPrimitive_TriangleStrip, vertex_description, 4, vertices);
  quad_.Create(std::move(quad_mesh));

  // Create the shader we can reuse for texture rendering.
  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/pass_through.glsl")) {
    pass_through_shader_.Create(std::move(source), quad_.vertex_description(),
                                quad_.primitive(), false);
  } else {
    LOG(0) << "Could not create pass through shader.";
  }

  // Create the shader we can reuse for solid rendering.
  source = std::make_unique<ShaderSource>();
  if (source->Load("engine/solid.glsl")) {
    solid_shader_.Create(std::move(source), quad_.vertex_description(),
                         quad_.primitive(), false);
  } else {
    LOG(0) << "Could not create solid shader.";
  }

  imgui_backend_.CreateRenderResources(renderer_.get());

  for (auto& t : textures_) {
    std::shared_ptr<Texture> texture = t.second.texture.lock();
    if (texture) {
      texture->SetRenderer(renderer_.get());
      ++async_work_count_;
      thread_pool_.PostTaskAndReplyWithResult<std::unique_ptr<Image>>(
          HERE, t.second.create_image,
          [&, ptr = texture](std::unique_ptr<Image> image) -> void {
            --async_work_count_;
            if (image)
              ptr->Update(std::move(image));
          });
    }
  }

  for (auto& s : shaders_) {
    std::shared_ptr<Shader> shader = s.second.shader.lock();
    if (shader) {
      shader->SetRenderer(renderer_.get());
      ++async_work_count_;
      thread_pool_.PostTaskAndReplyWithResult<std::unique_ptr<ShaderSource>>(
          HERE,
          [file_name = s.second.file_name]() -> std::unique_ptr<ShaderSource> {
            auto source = std::make_unique<ShaderSource>();
            if (!source->Load(file_name))
              return nullptr;
            return source;
          },
          [&, ptr = shader](std::unique_ptr<ShaderSource> source) -> void {
            --async_work_count_;
            if (source)
              ptr->Create(std::move(source), quad_.vertex_description(),
                          quad_.primitive(), false);
          });
    }
  }
}

void Engine::WaitForAsyncWork() {
  while (async_work_count_ > 0) {
    TaskRunner::GetThreadLocalTaskRunner()->RunTasks();
    platform_->Update();
  }
}

void Engine::ShowStats() {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;
  ImGui::Begin("Stats", nullptr, window_flags);
  ImGui::Text("%s", renderer_->GetDebugName());
  ImGui::Text("%d fps", fps_);
  ImGui::End();
}

}  // namespace eng
