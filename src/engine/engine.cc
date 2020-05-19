#include "engine.h"
#include "../base/log.h"
#include "../base/random.h"
#include "platform/platform.h"
#include "image.h"
#include "shader_source.h"
#include "font.h"
#include "mesh.h"
#include "renderer/renderer.h"
#include "renderer/render_command.h"
#include "game.h"
#include "game_factory.h"
#include "input_event.h"
#include <algorithm>

using base::Vector2;
using base::Matrix4x4;

namespace eng {

Engine::Engine() = default;

Engine::~Engine() = default;

Engine& Engine::Get() {
  static Engine engine;
  return engine;
}

bool Engine::Init(Platform* platform) {
  base::RandomInit();

  platform_ = platform;

  renderer_ = platform->GetRenderer();
  renderer_->SetContextLostCB(std::bind(&Engine::ContextLost, this));

  if (GetScreenWidth() > GetScreenHeight()) {
    float ratio = (float)GetScreenWidth() / (float)GetScreenHeight();
    screen_size_ = {ratio * 2.0f, 2.0f};
  } else {
    float ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    screen_size_ = {2.0f, ratio * 2.0f};
  }

  system_font_ = GetFontAsset("engine/Roboto-Regular.ttf");

  if (!CreateRenderResources())
    return false;

  game_ = eng::GameFactoryBase::CreateGame("");
  if (!game_) {
    printf("No game found to run.\n");
    return false;
  }

  if (!game_->Initialize()) {
    LOG << "Failed to initialize the game.";
    return false;
  }

#if 0
  ShowStats(true);
#endif

  return true;
}

void Engine::Shutdown() {
  game_.reset();
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  game_->Update(delta_time);
  if (stats_.IsVisible())
    PrintStats();
  KillUnusedResources(delta_time);
}

void Engine::Draw(float frame_frac) {
  Clear();
  auto cmd = std::make_unique<CmdEableBlend>();
  renderer_->EnqueueCommand(std::make_unique<CmdEableBlend>());

  game_->Draw(frame_frac);

  if (stats_.IsVisible())
    stats_.Draw();

  Present();
}

void Engine::Clear() {
  static float grey = 0.0f;
#if 0
  // Pulsate the clear color to make it more visible if we see it.
  grey += 0.01f;
  if (grey > 1.0f)
    grey = 0.0f;
#endif
  auto cmd = std::make_unique<CmdClear>();
  cmd->rgba = {grey, grey, grey, 1.0f};
  renderer_->EnqueueCommand(std::move(cmd));
}

void Engine::Present() {
  renderer_->Present();
}

void Engine::TrimMemory() {
  renderer_->TrimMemory();
}

Vector2 Engine::ToScale(const Vector2& vec) {
  return GetScreenSize() * vec /
      Vector2((float)GetScreenWidth(),
              (float)GetScreenHeight());
}

Vector2 Engine::ToPosition(const Vector2& vec) {
  return ToScale(vec) - GetScreenSize() / 2.0f;
}

std::shared_ptr<const Mesh> Engine::GetMeshAsset(const std::string& name) {
  auto it = mesh_assets_.find(name);
  if (it != mesh_assets_.end())
    return it->second;

  auto mesh = std::make_shared<Mesh>();
  if (!mesh->Load(name.c_str()))
    return nullptr;
  mesh->SetImmutable();

  mesh_assets_[name] = mesh;
  return mesh;
}

std::shared_ptr<const Image> Engine::GetImageAsset(const std::string& name) {
  auto it = image_assets_.find(name);
  if (it != image_assets_.end())
    return it->second;

  auto image = std::make_shared<Image>();
  if (!image->Load(name.c_str())) {
    auto it = image_assets_.find("unknown_image_asset");
    if (it != image_assets_.end()) {
      image = it->second;
    } else {
      image->Create(8, 8);
      image->Clear({0, 0, 0, 1});
      image->SetName("unknown_image_asset");
    }
  }
  image->SetImmutable();

  image_assets_[name] = image;
  return image;
}

std::shared_ptr<const ShaderSource> Engine::GetShaderAsset(const std::string& name) {
  auto it = shader_source_assets_.find(name);
  if (it != shader_source_assets_.end())
    return it->second;

  auto shader_source = std::make_shared<ShaderSource>();
  if (!shader_source->Load(name.c_str()))
    return nullptr;
  shader_source->SetImmutable();

  shader_source_assets_[name] = shader_source;
  return shader_source;
}

std::shared_ptr<Font> Engine::GetFontAsset(const std::string& name) {
  auto it = font_assets_.find(name);
  if (it != font_assets_.end())
    return it->second;

  auto font = std::make_shared<Font>();
  if (!font->Load(name.c_str())) {
    auto it = font_assets_.find("null_font_asset");
    if (it != font_assets_.end())
      font = it->second;
    else
      font->SetName("null_font_asset");
  }

  font_assets_[name] = font;
  return font;
}

int Engine::AcquireTextureResource(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  int resource_id = 0;
  if (image->GetName().empty()) {
    resource_id = ++last_texture_resource_id_;
  } else {
    auto it = texture_resources_.find(image->GetName());
    if (it != texture_resources_.end()) {
      ++(it->second.ref_count);
      return it->second.resource_id;
    }
    resource_id = ++last_texture_resource_id_;
    texture_resources_[image->GetName()] = {resource_id, 1};
    DLOG << "AcquireTextureResource - Create! asset: " << image->GetName()
         << ", resource_id: " << resource_id;
  }

  auto cmd = std::make_unique<CmdUpdateTexture>();
  cmd->id = resource_id;
  cmd->image = image;
  renderer_->EnqueueCommand(std::move(cmd));
  return resource_id;
}

void Engine::ReturnTextureResource(int resource_id) {
  auto it = std::find_if(texture_resources_.begin(), texture_resources_.end(),
      [resource_id](auto& p){ return p.second.resource_id == resource_id; });
  if (it != texture_resources_.end()) {
    assert(it->second.ref_count > 0);
    if (--(it->second.ref_count) > 0)
      return;
    it->second.time_to_die_ = 5;
    return;
  }
  auto cmd = std::make_unique<CmdDestoryTexture>();
  cmd->id = resource_id;
  renderer_->EnqueueCommand(std::move(cmd));
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
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

void Engine::EnqueueRenderCommand(std::unique_ptr<RenderCommand> cmd) {
  renderer_->EnqueueCommand(std::move(cmd));
}

int Engine::GetScreenWidth() const {
  return renderer_->screen_width();
}

int Engine::GetScreenHeight() const {
  return renderer_->screen_height();
}

const  Matrix4x4& Engine::GetProjectionMarix() const {
  return renderer_->projection();
}

int Engine::GetDeviceDpi() const {
  return platform_->GetDeviceDpi();
}

const std::string& Engine::GetRootPath() const {
  return platform_->GetRootPath();
}

void Engine::ContextLost() {
  texture_resources_.clear();
  last_texture_resource_id_ = 0;

  pass_through_shader_.Invalidate();
  solid_shader_.Invalidate();
  quad_.Invalidate();
  CreateRenderResources();

  stats_.ContextLost();

  game_->ContextLost();
}

bool Engine::CreateRenderResources() {
  // Create the quad geometry we can reuse for all sprites.
  auto quad_mesh = GetMeshAsset("engine/quad.mesh");
  if (!quad_mesh) {
    LOG << "Could not create quad mesh.";
    return false;
  }
  quad_.Create(quad_mesh);

  // Create the shader we can reuse for texture rendering.
  auto pts_code = GetShaderAsset("engine/pass_through");
  if (!pts_code) {
    LOG << "Could not create pass through shader.";
    return false;
  }
  pass_through_shader_.Create(pts_code, quad_.vertex_description());

  // Create the shader we can reuse for solid rendering.
  auto ss_code = GetShaderAsset("engine/solid");
  if (!ss_code) {
    LOG << "Could not create solid shader.";
    return false;
  }
  solid_shader_.Create(ss_code, quad_.vertex_description());

  return true;
}

void Engine::KillUnusedResources(float delta_time) {
  for (auto it = texture_resources_.begin(); it != texture_resources_.end();
      ++it) {
    if (it->second.ref_count > 0)
      continue;

    it->second.time_to_die_ -= delta_time;
    if (it->second.time_to_die_ <= 0.0f) {
      DLOG << "KillUnusedResources - Destroy! resource_id: "<<
          it->second.resource_id;

      auto cmd = std::make_unique<CmdDestoryTexture>();
      cmd->id = it->second.resource_id;
      renderer_->EnqueueCommand(std::move(cmd));

      it = texture_resources_.erase(it);
    }
  }
}

void Engine::ShowStats(bool show) {
  stats_.SetVisible(show);
  if (show)
    PrintStats();
}

void Engine::PrintStats() {
  constexpr int width = 300;
  std::vector<std::string> lines;
  std::string line = "frames dropped: ";
  line += std::to_string(renderer_->num_frames_dropped());
  lines.push_back(line);
  line = "global queue: ";
  line += std::to_string(renderer_->global_queue_size());
  lines.push_back(line);
  line = "render queue: ";
  line += std::to_string(renderer_->render_queue_size());
  lines.push_back(line);

  constexpr int margin = 3;
  int line_height = system_font_->GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_shared<Image>();
  image->Create(image_width, image_height);
  image->Clear({1, 1, 1, 0.08f});

  int y = margin;
  for (auto& text : lines) {
    system_font_->Print(margin, y + margin, text.c_str(), image->GetBuffer(),
        image->GetWidth());
    y += line_height + margin;
  }

  image->SetImmutable();
  stats_.Create(image);
  stats_.AutoScale();

  Vector2 pos = (GetScreenSize() / 2 - stats_.GetScale() / 2);
  pos -= Vector2(0.02f, 0.1f);
  stats_.SetOffset(pos * Vector2(-1, 1));
}

}  // namespace eng
