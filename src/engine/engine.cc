#include "engine.h"

#include "../base/log.h"
#include "../third_party/texture_compressor/texture_compressor.h"
#include "animator.h"
#include "audio/audio.h"
#include "audio/audio_resource.h"
#include "drawable.h"
#include "font.h"
#include "game.h"
#include "game_factory.h"
#include "image.h"
#include "image_quad.h"
#include "input_event.h"
#include "mesh.h"
#include "platform/platform.h"
#include "renderer/geometry.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/texture.h"
#include "shader_source.h"

using namespace base;

namespace eng {

Engine* Engine::singleton = nullptr;

Engine::Engine(Platform* platform, Renderer* renderer, Audio* audio)
    : platform_(platform), renderer_(renderer), audio_(audio) {
  DCHECK(!singleton);
  singleton = this;

  renderer_->SetContextLostCB(std::bind(&Engine::ContextLost, this));

  quad_ = CreateRenderResource<Geometry>();
  pass_through_shader_ = CreateRenderResource<Shader>();
  solid_shader_ = CreateRenderResource<Shader>();

  stats_ = std::make_unique<ImageQuad>();
  stats_->SetZOrder(std::numeric_limits<int>::max());
}

Engine::~Engine() {
  game_.reset();
  stats_.reset();

  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

bool Engine::Initialize() {
  // Normalize viewport.
  if (GetScreenWidth() > GetScreenHeight()) {
    float aspect_ratio = (float)GetScreenWidth() / (float)GetScreenHeight();
    LOG << "aspect ratio: " << aspect_ratio;
    screen_size_ = {aspect_ratio * 2.0f, 2.0f};
    projection_.CreateOrthoProjection(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
  } else {
    float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    LOG << "aspect_ratio: " << aspect_ratio;
    screen_size_ = {2.0f, aspect_ratio * 2.0f};
    projection_.CreateOrthoProjection(-1.0, 1.0, -aspect_ratio, aspect_ratio);
  }

  LOG << "image scale factor: " << GetImageScaleFactor();

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

  system_font_ = std::make_unique<Font>();
  system_font_->Load("engine/RobotoMono-Regular.ttf");

  if (!CreateRenderResources())
    return false;

  SetImageSource("stats_tex", std::bind(&Engine::PrintStats, this));

  game_ = GameFactoryBase::CreateGame("");
  if (!game_) {
    LOG << "No game found to run.";
    return false;
  }

  if (!game_->Initialize()) {
    LOG << "Failed to initialize the game.";
    return false;
  }

  return true;
}

void Engine::Shutdown() {
  LOG << "Shutting down engine.";
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;

  for (auto d : animators_)
    d->Update(delta_time);

  game_->Update(delta_time);

  // Destroy unused textures.
  for (auto& t : textures_) {
    if (!t.second.persistent && t.second.texture.use_count() == 1)
      t.second.texture->Destroy();
  }

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
    d->EvalAnim(time_step_ * frame_frac);

  drawables_.sort(
      [](auto& a, auto& b) { return a->GetZOrder() < b->GetZOrder(); });

  renderer_->PrepareForDrawing();

  for (auto d : drawables_) {
    if (d->IsVisible())
      d->Draw(frame_frac);
  }

  renderer_->Present();
}

void Engine::LostFocus() {
  audio_->Suspend();

  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus(bool from_interstitial_ad) {
  audio_->Resume();

  if (game_)
    game_->GainedFocus(from_interstitial_ad);
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
  std::shared_ptr<Texture> texture;
  auto it = textures_.find(asset_name);
  if (it != textures_.end()) {
    texture = it->second.texture;
    it->second.create_image = create_image;
    it->second.persistent = persistent;
  } else {
    texture = CreateRenderResource<Texture>();
    textures_[asset_name] = {texture, create_image, persistent};
  }

  if (persistent) {
    auto image = create_image();
    if (image)
      texture->Update(std::move(image));
    else
      texture->Destroy();
  }
}

void Engine::RefreshImage(const std::string& asset_name) {
  auto it = textures_.find(asset_name);
  if (it == textures_.end())
    return;

  auto image = it->second.create_image();
  if (image)
    it->second.texture->Update(std::move(image));
  else
    it->second.texture->Destroy();
}

std::shared_ptr<Texture> Engine::GetTexture(const std::string& asset_name) {
  auto it = textures_.find(asset_name);
  if (it != textures_.end()) {
    if (!it->second.texture->IsValid())
      RefreshImage(it->first);
    return it->second.texture;
  }

  std::shared_ptr<Texture> texture = CreateRenderResource<Texture>();
  textures_[asset_name] = {texture};

  return texture;
}

std::unique_ptr<AudioResource> Engine::CreateAudioResource() {
  return std::make_unique<AudioResource>(audio_);
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  if (replaying_)
    return;

  switch (event->GetType()) {
    case InputEvent::kDragEnd:
      if (((GetScreenSize() / 2) * 0.9f - event->GetVector()).Length() <=
          0.25f) {
        SetSatsVisible(!stats_->IsVisible());
        // TODO: Enqueue DragCancel so we can consume this event.
      }
      break;
    case InputEvent::kKeyPress:
      if (event->GetKeyPress() == 's') {
        SetSatsVisible(!stats_->IsVisible());
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
    random_ = Random();
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
    random_ = Random(replay_data_.root()["seed"].asUInt());
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
  audio_->SetEnableAudio(enable);
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

int Engine::GetAudioHardwareSampleRate() {
  return audio_->GetHardwareSampleRate();
}

bool Engine::IsMobile() const {
  return platform_->mobile_device();
}

std::unique_ptr<RenderResource> Engine::CreateRenderResourceInternal(
    RenderResourceFactoryBase& factory) {
  return renderer_->CreateResource(factory);
}

void Engine::ContextLost() {
  CreateRenderResources();

  for (auto& t : textures_)
    RefreshImage(t.first);

  game_->ContextLost();
}

bool Engine::CreateRenderResources() {
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
  if (!source->Load("engine/pass_through.glsl")) {
    LOG << "Could not create pass through shader.";
    return false;
  }
  pass_through_shader_->Create(std::move(source), quad_->vertex_description(),
                               quad_->primitive());

  // Create the shader we can reuse for solid rendering.
  source = std::make_unique<ShaderSource>();
  if (!source->Load("engine/solid.glsl")) {
    LOG << "Could not create solid shader.";
    return false;
  }
  solid_shader_->Create(std::move(source), quad_->vertex_description(),
                        quad_->primitive());

  return true;
}

void Engine::SetSatsVisible(bool visible) {
  stats_->SetVisible(visible);
  if (visible)
    stats_->Create("stats_tex");
  else
    stats_->Destory();
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
  // line = "cmd: ";
  // line += std::to_string(renderer_->global_queue_size() +
  //                        renderer_->render_queue_size());
  // lines.push_back(line);

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
