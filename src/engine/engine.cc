#include "engine.h"

#include "../base/log.h"
#include "../base/worker.h"
#include "../third_party/texture_compressor/texture_compressor.h"
#include "audio/audio.h"
#include "audio/audio_resource.h"
#include "font.h"
#include "game.h"
#include "game_factory.h"
#include "image.h"
#include "input_event.h"
#include "mesh.h"
#include "platform/platform.h"
#include "renderer/geometry.h"
#include "renderer/render_command.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/texture.h"
#include "shader_source.h"

using namespace base;

namespace eng {

Engine* Engine::singleton = nullptr;

Engine::Engine(Platform* platform, Renderer* renderer, Audio* audio)
    : platform_(platform), renderer_(renderer), audio_(audio) {
  assert(!singleton);
  singleton = this;

  renderer_->SetContextLostCB(std::bind(&Engine::ContextLost, this));

  quad_ = CreateRenderResource<Geometry>();
  pass_through_shader_ = CreateRenderResource<Shader>();
  solid_shader_ = CreateRenderResource<Shader>();
}

Engine::~Engine() {
  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

bool Engine::Initialize() {
  // The orthogonal viewport is (-1.0 .. 1.0) for the short edge of the screen.
  // For the long endge, it's calculated from aspect ratio.
  if (GetScreenWidth() > GetScreenHeight()) {
    float aspect_ratio = (float)GetScreenWidth() / (float)GetScreenHeight();
    LOG << "aspect ratio: " << aspect_ratio;
    screen_size_ = {aspect_ratio * 2.0f, 2.0f};
    projection_ = base::Ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
  } else {
    float aspect_ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    LOG << "aspect_ratio: " << aspect_ratio;
    screen_size_ = {2.0f, aspect_ratio * 2.0f};
    projection_ = base::Ortho(-1.0, 1.0, -aspect_ratio, aspect_ratio);
  }

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

  game_ = GameFactoryBase::CreateGame("");
  if (!game_) {
    printf("No game found to run.\n");
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

  audio_->Update();
  renderer_->Update();

  game_->Update(delta_time);

  fps_seconds_ += delta_time;
  if (fps_seconds_ >= 1) {
    fps_ = renderer_->GetAndResetFPS();
    fps_seconds_ = 0;
  }

  if (stats_.IsVisible())
    PrintStats();
}

void Engine::Draw(float frame_frac) {
  auto cmd = std::make_unique<CmdClear>();
  cmd->rgba = {0, 0, 0, 1};
  renderer_->EnqueueCommand(std::move(cmd));
  renderer_->EnqueueCommand(std::make_unique<CmdEableBlend>());

  game_->Draw(frame_frac);

  if (stats_.IsVisible())
    stats_.Draw();

  renderer_->EnqueueCommand(std::make_unique<CmdPresent>());
}

void Engine::LostFocus() {
  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus() {
  if (game_)
    game_->GainedFocus();
}

void Engine::Exit() {
  platform_->Exit();
}

Vector2 Engine::ToScale(const Vector2& vec) {
  return GetScreenSize() * vec /
         Vector2((float)GetScreenWidth(), (float)GetScreenHeight());
}

Vector2 Engine::ToPosition(const Vector2& vec) {
  return ToScale(vec) - GetScreenSize() / 2.0f;
}

std::shared_ptr<AudioResource> Engine::CreateAudioResource() {
  return std::make_shared<AudioResource>(audio_);
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  switch (event->GetType()) {
    case InputEvent::kTap:
      if (((GetScreenSize() / 2) * 0.9f - event->GetVector(0)).Magnitude() <=
          0.25f) {
        SetSatsVisible(!stats_.IsVisible());
        // Consume event.
        return;
      }
      break;
    case InputEvent::kKeyPress:
      if (event->GetKeyPress() == 's') {
        SetSatsVisible(!stats_.IsVisible());
        // Consume event.
        return;
      }
      break;
    case InputEvent::kDrag:
      if (stats_.IsVisible()) {
        if ((stats_.GetOffset() - event->GetVector(0)).Magnitude() <=
            stats_.GetScale().y)
          stats_.SetOffset(event->GetVector(0));
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
  if (!input_queue_.empty()) {
    event.swap(input_queue_.front());
    input_queue_.pop_front();
  }
  return event;
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

const std::string& Engine::GetRootPath() const {
  return platform_->GetRootPath();
}

size_t Engine::GetAudioSampleRate() {
  return audio_->GetSampleRate();
}

bool Engine::IsMobile() const {
  return platform_->mobile_device();
}

std::shared_ptr<RenderResource> Engine::CreateRenderResourceInternal(
    RenderResourceFactoryBase& factory) {
  return renderer_->CreateResource(factory);
}

void Engine::ContextLost() {
  CreateRenderResources();

  game_->ContextLost();
}

bool Engine::CreateRenderResources() {
  // Create the quad geometry we can reuse for all sprites.
  auto quad_mesh = std::make_unique<Mesh>();
  if (!quad_mesh->Load("engine/quad.mesh")) {
    LOG << "Could not create quad mesh.";
    return false;
  }
  quad_->Create(std::move(quad_mesh));

  // Create the shader we can reuse for texture rendering.
  auto source = std::make_unique<ShaderSource>();
  if (!source->Load("engine/pass_through.glsl")) {
    LOG << "Could not create pass through shader.";
    return false;
  }
  pass_through_shader_->Create(std::move(source), quad_->vertex_description());

  // Create the shader we can reuse for solid rendering.
  source = std::make_unique<ShaderSource>();
  if (!source->Load("engine/solid.glsl")) {
    LOG << "Could not create solid shader.";
    return false;
  }
  solid_shader_->Create(std::move(source), quad_->vertex_description());

  return true;
}

void Engine::SetSatsVisible(bool visible) {
  stats_.SetVisible(visible);
  if (visible)
    stats_.Create(CreateRenderResource<Texture>());
  else
    stats_.Destory();
}

void Engine::PrintStats() {
  constexpr int width = 200;
  std::vector<std::string> lines;
  std::string line;
  line = "fps: ";
  line += std::to_string(fps_);
  lines.push_back(line);
  line = "cmd: ";
  line += std::to_string(renderer_->global_queue_size() +
                         renderer_->render_queue_size());
  lines.push_back(line);

  constexpr int margin = 5;
  int line_height = system_font_->GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_unique<Image>();
  image->Create(image_width, image_height);
  image->Clear({1, 1, 1, 0.08f});

  Worker worker(2);
  int y = margin;
  for (auto& text : lines) {
    worker.Enqueue(std::bind(&Font::Print, system_font_.get(), margin, y,
                             text.c_str(), image->GetBuffer(),
                             image->GetWidth()));
    y += line_height + margin;
  }
  worker.Join();

  stats_.GetTexture()->Update(std::move(image));
  stats_.AutoScale();
}

}  // namespace eng
