#include "engine.h"
#include "../base/log.h"
#include "../base/random.h"
#include "asset_manager/image.h"
#include "renderer/render_command.h"
#include "game.h"
#include "game_factory.h"
#include "input_event.h"
#include <algorithm>

namespace eng {

Engine& Engine::Get() {
  static Engine engine;
  return engine;
}

bool Engine::Init() {
  RandomInit();

  renderer_.SetDelegate(this);

  if (!font_.Create("Roboto-Regular.ttf")) {
    LOG << "Failed to create the font.";
    return false;
  }

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
  renderer_.EnableBlend();

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
  renderer_.Clear({grey, grey, grey, 1.0f});
}

void Engine::Present() {
  renderer_.Present();
}

void Engine::TrimMemory() {
  renderer_.TrimMemory();
}

// TODO: do once during initialization.
Vector2 Engine::GetScreenSize() {
  if (GetScreenWidth() > GetScreenHeight()) {
    float ratio = (float)GetScreenWidth() / (float)GetScreenHeight();
    return Vector2(ratio * 2.0f, 2.0f);
  } else {
    float ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
    return Vector2(2.0f, ratio * 2.0f);
  }
}

Vector2 Engine::ToScale(const Vector2& vec) {
  return GetScreenSize() * vec /
      Vector2((float)GetScreenWidth(),
              (float)GetScreenHeight());
}

Vector2 Engine::ToPosition(const Vector2& vec) {
  return ToScale(vec) - GetScreenSize() / 2.0f;
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

  auto cmd = std::make_unique<CmdCreateTexture>();
  cmd->id = resource_id;
  cmd->image = image;
  renderer_.EnqueueCommand(std::move(cmd));
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
  renderer_.EnqueueCommand(std::move(cmd));
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
  // Create the shader we can reuse for texture rendering.
  const char* vertex_description = "p2f;t2f";
  if (!pass_through_shader_.Create("shaders/pass_through",
      vertex_description)) {
    LOG << "Could not create pass through shader.";
    return false;
  }

  // Create the shader we can reuse for solid rendering.
  if (!solid_shader_.Create("shaders/solid", vertex_description)) {
    LOG << "Could not create solid pass through shader.";
    return false;
  }

  // Create the quad geometry we can reuse for all sprites.
  // This creates a normalized unit sized quad.
  static const float vertices[] = {
    -0.5f, -0.5f, 0.0f, 1.0f,
     0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 0.0f
  };
  quad_.Create(GL_TRIANGLE_STRIP, vertex_description, 4, vertices);

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
      renderer_.EnqueueCommand(std::move(cmd));

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
  line += std::to_string(eng::Engine::Get().GetRenderer().num_frames_dropped());
  lines.push_back(line);
  line = "global queue: ";
  line += std::to_string(eng::Engine::Get().GetRenderer().global_queue_size());
  lines.push_back(line);
  line = "render queue: ";
  line += std::to_string(eng::Engine::Get().GetRenderer().render_queue_size());
  lines.push_back(line);

  constexpr int margin = 3;
  int line_height = font_.GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_shared<Image>();
  image->Create(image_width, image_height);
  float c[4] = {1, 1, 1, 0.08f};
  image->Clear(c);

  int y = margin;
  for (auto& text : lines) {
    font_.Print(margin, y + margin, text.c_str(), image->GetBuffer(),
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
