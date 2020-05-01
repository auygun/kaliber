#include "image_quad.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../base/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include <cassert>

namespace engine {

bool ImageQuad::Create(std::shared_ptr<const Image> image,
                       std::array<int, 2> num_frames) {
  assert(num_frames_[0] > 0 && num_frames_[1] > 0);

  if (!texture_.Create(image))
    return false;

  int frame_width = image->GetOriginalWidth() / num_frames[0];
  int frame_height = image->GetOriginalHeight() / num_frames[1];
  SetScale(engine::Engine::Get().ToScale(Vector2(frame_width, frame_height)));
  tex_scale_ = {
    (float)frame_width / (float)image->GetWidth(),
    (float)frame_height / (float)image->GetHeight()
  };
  num_frames_ = std::move(num_frames);
  return true;
}

size_t ImageQuad::GetNumFrames() {
  return num_frames_[0] * num_frames_[1];
}

size_t ImageQuad::GetCurrentFrame() {
  return current_frame_;
}

void ImageQuad::SetCurrentFrame(size_t frame) {
  assert(frame < GetNumFrames());
  current_frame_ = frame;
}

void ImageQuad::Draw() {
  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset());
  shader.SetUniform("scale", scale());
  shader.SetUniform("center", center());
  shader.SetUniform("rotation", rotation());
  shader.SetUniform("tex_offset", GetUVOffset(current_frame_));
  shader.SetUniform("tex_scale", tex_scale_);
  shader.SetUniform("projection", Engine::Get().GetRenderer().projection());
  shader.SetUniform("color", color());
  shader.SetUniform("texture", 0);

  quad.Draw();
}

Vector2 ImageQuad::GetUVOffset(int frame) {
  assert(frame < num_frames_[0] * num_frames_[1]);
  if (num_frames_[0] == 1 && num_frames_[1] == 1)
    return {0, 0};
  return {(float)(frame % num_frames_[0]), (float)(frame / num_frames_[0])};
}

}  // namespace engine
