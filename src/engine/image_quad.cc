#include "image_quad.h"
#include "../base/log.h"
#include "../base/image.h"
#include "../base/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace engine {

bool ImageQuad::Create(const std::string& asset_name, const Vector2& offset) {
  auto image = std::make_unique<Image>();
  if (!image->Load(asset_name.c_str()))
    return false;

  offset_ = offset;
  scale_ = engine::Engine::Get().ToScale(image->GetWidth(), image->GetHeight());

  return texture_.Create(std::move(image));
}

bool ImageQuad::Print(const std::string& text, const Vector2& offset) {
  Fontx& font = engine::Engine::Get().GetFont();

  int w, h;
  font.CalculateBoundingBox(text.c_str(), w, h);

  auto image = std::make_unique<Image>();
  if (!image->Create(w, h))
    return false;
  float c[4] = {0, 0, 0, 0};
  image->Clear(c);

  font.Print(0, 0, text.c_str(), image->GetBuffer(), image->GetWidth());

  offset_ = offset;
  scale_ = engine::Engine::Get().ToScale(w, h);

  return texture_.Create(std::move(image));
}

bool ImageQuad::Print(const std::vector<std::string> lines, int width,
                      const Vector2& offset) {
  Fontx& font = engine::Engine::Get().GetFont();
  constexpr int margin = 3;
  int line_height = font.GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_unique<Image>();
  if (!image->Create(image_width, image_height))
    return false;
  float c[4] = {1, 1, 1, 0.08f};
  image->Clear(c);

  int y = margin;
  for (auto& text : lines) {
    font.Print(margin, y + margin, text.c_str(), image->GetBuffer(),
        image->GetWidth());
    y += line_height + margin;
  }

  offset_ = offset;
  scale_ = engine::Engine::Get().ToScale(image_width, image_height);

  return texture_.Create(std::move(image));
}

void ImageQuad::Draw(const Vector2& offset) {
  Vector2 draw_offset = offset_ + offset;

  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", draw_offset);
  shader.SetUniform("scale", scale_);
  shader.SetUniform("tileColor", Vector3(1, 1, 1));
  shader.SetUniform("tileImage", 0);

  quad.Draw();
}

}  // namespace engine
