#include "image_quad.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../base/font.h"
#include "engine.h"
#include "../platform/platform.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include <cassert>

namespace eng {

void ImageQuad::Create(std::shared_ptr<const Image> image,
                       std::array<int, 2> num_frames) {
  assert(num_frames_[0] > 0 && num_frames_[1] > 0);

  texture_.Update(image);

  frame_width_ = image->GetOriginalWidth() / num_frames[0];
  frame_height_ = image->GetOriginalHeight() / num_frames[1];

  tex_scale_ = {
    (float)frame_width_ / (float)image->GetWidth(),
    (float)frame_height_ / (float)image->GetHeight()
  };
  num_frames_ = std::move(num_frames);
}

void ImageQuad::AutoScale() {
  SetScale(eng::Engine::Get().ToScale(Vector2(frame_width_, frame_height_)));
  Scale((float)Platform::Get().GetDeviceDpi() / 200.0f);
}

void ImageQuad::Translate(const Vector2& offset) {
  offset_ += offset;
}

void ImageQuad::Scale(const Vector2& scale) {
  scale_ *= scale;
}

void ImageQuad::Scale(float scale) {
  scale_ *= scale;
}

void ImageQuad::Rotate(float angle) {
  theta_ += angle;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

void ImageQuad::SetTheta(float theta) {
  theta_ = theta;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

void ImageQuad::SetFrame(size_t frame) {
  assert(frame < GetNumFrames());
  current_frame_ = frame;
}

size_t ImageQuad::GetNumFrames() {
  return num_frames_[0] * num_frames_[1];
}

void ImageQuad::Draw() {
  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset_);
  shader.SetUniform("scale", scale_);
  shader.SetUniform("pivot", pivot_);
  shader.SetUniform("rotation", rotation_);
  shader.SetUniform("tex_offset", GetUVOffset(current_frame_));
  shader.SetUniform("tex_scale", tex_scale_);
  shader.SetUniform("projection", Engine::Get().GetProjectionMarix());
  shader.SetUniform("color", color_);
  shader.SetUniform("texture", 0);

  quad.Draw();
}

void ImageQuad::ContextLost() {
  texture_.Invalidate();
}

// Return the uv offset for the given frame.
Vector2 ImageQuad::GetUVOffset(int frame) {
  assert(frame < num_frames_[0] * num_frames_[1]);
  if (num_frames_[0] == 1 && num_frames_[1] == 1)
    return {0, 0};
  return {(float)(frame % num_frames_[0]), (float)(frame / num_frames_[0])};
}

}  // namespace eng
