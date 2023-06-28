#include "engine/engine.h"

#include "base/log.h"
#include "base/task_runner.h"
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
#include "engine/image_quad.h"
#include "engine/input_event.h"
#include "engine/platform/platform.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/opengl/renderer_opengl.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/vulkan/renderer_vulkan.h"
#include "third_party/texture_compressor/texture_compressor.h"

using namespace base;

namespace eng {

extern void KaliberMain(Platform* platform) {
  TaskRunner::CreateThreadLocalTaskRunner();
  Engine(platform).Run();
}

Engine* Engine::singleton = nullptr;

Engine::Engine(Platform* platform)
    : platform_(platform),
      audio_mixer_{std::make_unique<AudioMixer>()},
      quad_{std::make_unique<Geometry>(nullptr)},
      pass_through_shader_{std::make_unique<Shader>(nullptr)},
      solid_shader_{std::make_unique<Shader>(nullptr)} {
  DCHECK(!singleton);
  singleton = this;

  platform_->SetObserver(this);

  stats_ = std::make_unique<ImageQuad>();
}

Engine::~Engine() {
  LOG(0) << "Shutting down engine.";

  thread_pool_.CancelTasks();
  thread_pool_.Shutdown();

  game_.reset();
  stats_.reset();
  textures_.clear();
  shaders_.clear();
  quad_.reset();
  pass_through_shader_.reset();
  solid_shader_.reset();
  renderer_.reset();
  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

void Engine::Run() {
  Initialize();

  timer_.Reset();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;) {
    TaskRunner::GetThreadLocalTaskRunner()->RunTasks();

    platform_->Update();
    if (platform_->should_exit())
      return;

    if (!renderer_->IsInitialzed()) {
      timer_.Reset();
      input_queue_.clear();
      continue;
    }

    Draw(frame_frac);

    // Accumulate time.
    timer_.Update();
    accumulator += timer_.GetSecondsPassed();

    // Subdivide the frame time using fixed time steps.
    while (accumulator >= time_step_) {
      Update(time_step_);
      accumulator -= time_step_;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step_;
  }
}

void Engine::Initialize() {
  LOG(0) << "Initializing engine.";

  thread_pool_.Initialize();

  CreateRendererInternal(RendererType::kVulkan);

  // Normalize viewport.
  if (GetScreenWidth() > GetScreenHeight()) {
    float aspect_ratio = (float)GetScreenWidth() / (float)GetScreenHeight();
    LOG(0) << "aspect ratio: " << aspect_ratio;
    screen_size_ = {aspect_ratio * 2.0f, 2.0f};
    projection_.CreateOrthoProjection(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
  } else {
    float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    LOG(0) << "aspect_ratio: " << aspect_ratio;
    screen_size_ = {2.0f, aspect_ratio * 2.0f};
    projection_.CreateOrthoProjection(-1.0, 1.0, -aspect_ratio, aspect_ratio);
  }

  LOG(0) << "image scale factor: " << GetImageScaleFactor();

  system_font_ = std::make_unique<Font>();
  system_font_->Load("engine/RobotoMono-Regular.ttf");

  SetImageSource("stats_tex", std::bind(&Engine::PrintStats, this));
  stats_->SetZOrder(std::numeric_limits<int>::max());

  game_ = GameFactoryBase::CreateGame("");
  CHECK(game_) << "No game found to run.";
  CHECK(game_->PreInitialize()) << "Failed to pre-initialize the game.";

  // Create resources and let the game finalize initialization.
  CreateRenderResources();
  WaitForAsyncWork();
  CHECK(game_->Initialize()) << "Failed to initialize the game.";
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;

  for (auto d : animators_)
    d->Update(delta_time);

  game_->Update(delta_time);

  fps_seconds_ += delta_time;
  if (fps_seconds_ >= 1) {
    fps_ = renderer_->GetAndResetFPS();
    fps_seconds_ = 0;
  }

  if (stats_->IsVisible()) {
    RefreshImage("stats_tex");
    stats_->AutoScale();
  }
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

Vector2f Engine::ToScale(const Vector2f& vec) {
  return GetScreenSize() * vec /
         Vector2f((float)GetScreenWidth(), (float)GetScreenHeight());
}

Vector2f Engine::ToPosition(const Vector2f& vec) {
  return ToScale(vec) - GetScreenSize() / 2.0f;
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
  if (textures_.contains(asset_name) && textures_[asset_name].use_count > 0) {
    DLOG(0) << "Texture in use: " << asset_name;
    return;
  }

  textures_[asset_name] = {std::make_unique<Texture>(renderer_.get()),
                           create_image, persistent, 0};
}

void Engine::RefreshImage(const std::string& asset_name) {
  auto it = textures_.find(asset_name);
  if (it == textures_.end()) {
    DLOG(0) << "Texture not found: " << asset_name;
    return;
  }

  if (it->second.persistent || it->second.use_count > 0) {
    auto image = it->second.create_image();
    if (image)
      it->second.texture->Update(std::move(image));
  }
}

Texture* Engine::AcquireTexture(const std::string& asset_name) {
  auto it = textures_.find(asset_name);
  if (it == textures_.end()) {
    DLOG(0) << "Texture not found: " << asset_name;
    return nullptr;
  }

  it->second.use_count++;
  if (!it->second.texture->IsValid()) {
    auto image = it->second.create_image();
    if (image)
      it->second.texture->Update(std::move(image));
  }
  return it->second.texture.get();
}

void Engine::ReleaseTexture(const std::string& asset_name) {
  auto it = textures_.find(asset_name);
  if (it == textures_.end()) {
    DLOG(0) << "Texture not found: " << asset_name;
    return;
  }

  DCHECK(it->second.use_count > 0);
  it->second.use_count--;
  if (!it->second.persistent && it->second.use_count == 0)
    it->second.texture->Destroy();
}

void Engine::SetShaderSource(const std::string& asset_name,
                             const std::string& file_name) {
  if (shaders_.contains(asset_name)) {
    DLOG(0) << "Shader already exists: " << asset_name;
    return;
  }

  shaders_[asset_name] = {std::make_unique<Shader>(renderer_.get()), file_name};
}

Shader* Engine::GetShader(const std::string& asset_name) {
  auto it = shaders_.find(asset_name);
  if (it == shaders_.end()) {
    DLOG(0) << "Shader not found: " << asset_name;
    return nullptr;
  }

  if (!it->second.shader->IsValid()) {
    auto source = std::make_unique<ShaderSource>();
    if (source->Load(it->second.file_name))
      it->second.shader->Create(std::move(source), quad_->vertex_description(),
                                quad_->primitive(), false);
  }

  return it->second.shader.get();
}

void Engine::AsyncLoadSound(const std::string& asset_name,
                            const std::string& file_name,
                            bool stream) {
  if (audio_buses_.contains(asset_name)) {
    DLOG(0) << "AudioBus already exists: " << asset_name;
    return;
  }

  auto sound = std::make_shared<Sound>();
  audio_buses_[asset_name] = sound;

  ++async_work_count_;
  thread_pool_.PostTaskAndReply(
      HERE, std::bind(&Sound::Load, sound, file_name, stream),
      [&]() -> void { --async_work_count_; });
}

std::shared_ptr<AudioBus> Engine::GetAudioBus(const std::string& asset_name) {
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

int Engine::GetScreenWidth() const {
  return renderer_->screen_width();
}

int Engine::GetScreenHeight() const {
  return renderer_->screen_height();
}

int Engine::GetDeviceDpi() const {
  return platform_->GetDeviceDpi();
}

float Engine::GetImageScaleFactor() const {
  float width_inch = static_cast<float>(renderer_->screen_width()) /
                     static_cast<float>(platform_->GetDeviceDpi());
  return 2.57143f / width_inch;
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
  renderer_->Initialize(platform_);
}

void Engine::OnWindowDestroyed() {
  renderer_->Shutdown();
}

void Engine::OnWindowResized(int width, int height) {
  if (width != renderer_->screen_width() ||
      height != renderer_->screen_height()) {
    renderer_->Shutdown();
    renderer_->Initialize(platform_);
  }
}

void Engine::LostFocus() {
  audio_mixer_->Suspend();

  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus(bool from_interstitial_ad) {
  timer_.Reset();
  audio_mixer_->Resume();

  if (game_)
    game_->GainedFocus(from_interstitial_ad);
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  if (replaying_)
    return;

  event->SetVector(ToPosition(event->GetVector()) * Vector2f(1, -1));

  switch (event->GetType()) {
    case InputEvent::kDragEnd:
      if (((GetScreenSize() / 2) * 0.9f - event->GetVector()).Length() <=
          0.25f) {
        SetStatsVisible(!stats_->IsVisible());
        // TODO: Enqueue DragCancel so we can consume this event.
      }
      break;
    case InputEvent::kKeyPress:
      if (event->GetKeyPress() == 's') {
        SetStatsVisible(!stats_->IsVisible());
        // Consume event.
        return;
      }
      break;
    case InputEvent::kDrag:
      if (stats_->IsVisible()) {
        if ((stats_->GetPosition() - event->GetVector()).Length() <=
            stats_->GetSize().y)
          stats_->SetPosition(event->GetVector());
        // TODO: Enqueue DragCancel so we can consume this event.
      }
      break;
    default:
      break;
  }

  input_queue_.push_back(std::move(event));
}

void Engine::CreateRendererInternal(RendererType type) {
  if ((dynamic_cast<RendererVulkan*>(renderer_.get()) &&
       type == RendererType::kVulkan) ||
      (dynamic_cast<RendererOpenGL*>(renderer_.get()) &&
       type == RendererType::kOpenGL))
    return;

  if (type == RendererType::kVulkan) {
    renderer_ =
        std::make_unique<RendererVulkan>(std::bind(&Engine::ContextLost, this));
  } else if (type == RendererType::kOpenGL) {
    renderer_ =
        std::make_unique<RendererOpenGL>(std::bind(&Engine::ContextLost, this));
  } else {
    NOTREACHED();
  }

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

void Engine::ContextLost() {
  CreateRenderResources();
  WaitForAsyncWork();
  input_queue_.clear();

  if (game_)
    game_->ContextLost();
}

void Engine::CreateRenderResources() {
  quad_->SetRenderer(renderer_.get());
  pass_through_shader_->SetRenderer(renderer_.get());
  solid_shader_->SetRenderer(renderer_.get());

  // This creates a normalized unit sized quad.
  static const char vertex_description[] = "p2f;t2f";
  static const float vertices[] = {
      -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  0.0f, 0.0f, 0.5f, 0.5f,  1.0f, 0.0f,
  };

  // Create the quad geometry we can reuse for all sprites.
  auto quad_mesh = std::make_unique<Mesh>();
  quad_mesh->Create(kPrimitive_TriangleStrip, vertex_description, 4, vertices);
  quad_->Create(std::move(quad_mesh));

  // Create the shader we can reuse for texture rendering.
  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/pass_through.glsl")) {
    pass_through_shader_->Create(std::move(source), quad_->vertex_description(),
                                 quad_->primitive(), false);
  } else {
    LOG(0) << "Could not create pass through shader.";
  }

  // Create the shader we can reuse for solid rendering.
  source = std::make_unique<ShaderSource>();
  if (source->Load("engine/solid.glsl")) {
    solid_shader_->Create(std::move(source), quad_->vertex_description(),
                          quad_->primitive(), false);
  } else {
    LOG(0) << "Could not create solid shader.";
  }

  for (auto& t : textures_) {
    t.second.texture->SetRenderer(renderer_.get());
    if (t.second.persistent || t.second.use_count > 0) {
      ++async_work_count_;
      thread_pool_.PostTaskAndReplyWithResult<std::unique_ptr<Image>>(
          HERE, t.second.create_image,
          [&,
           ptr = t.second.texture.get()](std::unique_ptr<Image> image) -> void {
            --async_work_count_;
            if (image)
              ptr->Update(std::move(image));
          });
    }
  }

  for (auto& s : shaders_) {
    s.second.shader->SetRenderer(renderer_.get());
    ++async_work_count_;
    thread_pool_.PostTaskAndReplyWithResult<std::unique_ptr<ShaderSource>>(
        HERE,
        [file_name = s.second.file_name]() -> std::unique_ptr<ShaderSource> {
          auto source = std::make_unique<ShaderSource>();
          if (!source->Load(file_name))
            return nullptr;
          return source;
        },
        [&, ptr = s.second.shader.get()](
            std::unique_ptr<ShaderSource> source) -> void {
          --async_work_count_;
          if (source)
            ptr->Create(std::move(source), quad_->vertex_description(),
                        quad_->primitive(), false);
        });
  }
}

void Engine::WaitForAsyncWork() {
  while (async_work_count_ > 0) {
    TaskRunner::GetThreadLocalTaskRunner()->RunTasks();
    platform_->Update();
  }
}

void Engine::SetStatsVisible(bool visible) {
  stats_->SetVisible(visible);
  if (visible)
    stats_->Create("stats_tex");
  else
    stats_->Destroy();
}

std::unique_ptr<Image> Engine::PrintStats() {
  if (!stats_->IsVisible())
    return nullptr;

  constexpr int width = 200;
  std::vector<std::string> lines;
  std::string line;
  line = "fps: ";
  line += std::to_string(fps_);
  lines.push_back(line);
  lines.push_back(renderer_->GetDebugName());

  constexpr int margin = 5;
  int line_height = system_font_->GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_unique<Image>();
  image->Create(image_width, image_height);
  image->Clear({1, 1, 1, 0.08f});

  int y = margin;
  for (auto& text : lines) {
    system_font_->Print(margin, y, text.c_str(), image->GetBuffer(),
                        image->GetWidth());
    y += line_height + margin;
  }

  return image;
}

}  // namespace eng
