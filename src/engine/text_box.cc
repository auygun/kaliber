#include "text_box.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../base/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace engine {

bool TextBox::Print(Fontx& font, const std::string& text) {
  int w, h;
  font.CalculateBoundingBox(text.c_str(), w, h);

  auto image = std::make_unique<Image>();
  if (!image->Create(w, h))
    return false;
  float c[4] = {0, 0, 0, 0};
  image->Clear(c);

  font.Print(0, 0, text.c_str(), image->GetBuffer(), image->GetWidth());

  SetScale(engine::Engine::Get().ToScale(Vector2(w, h)));

  return texture_.Create(std::move(image));
}

bool TextBox::Print(Fontx& font, const std::vector<std::string> lines,
                      int width) {
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

  SetScale(engine::Engine::Get().ToScale(Vector2(image_width, image_height)));

  return texture_.Create(std::move(image));
}

void TextBox::Draw() {
  if (!texture_.IsValid())
    return;

  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset());
  shader.SetUniform("scale", scale());
  shader.SetUniform("rotation", rotation());
  shader.SetUniform("uv_scale", Vector2(1, 1));
  shader.SetUniform("projection", engine::Engine::Get().GetRenderer().projection());
  shader.SetUniform("tile_color", Vector3(1, 1, 1));
  shader.SetUniform("tile_image", 0);

  quad.Draw();
}

}  // namespace engine
