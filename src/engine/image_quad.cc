#include "image_quad.h"

#include <cassert>

#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include "renderer/texture.h"

using namespace base;

namespace eng {

void ImageQuad::Create(std::shared_ptr<Texture> texture,
                       std::array<int, 2> num_frames,
                       int frame_width,
                       int frame_height) {
  texture_ = texture;
  num_frames_ = std::move(num_frames);
  frame_width_ = frame_width;
  frame_height_ = frame_height;
}

void ImageQuad::Destory() {
  texture_.reset();
}

void ImageQuad::AutoScale() {
  Vector2 dimensions = {GetFrameWidth(), GetFrameHeight()};
  SetScale(Engine::Get().ToScale(dimensions));
  Scale((float)Engine::Get().GetDeviceDpi() / 200.0f);
}

void ImageQuad::SetFrame(size_t frame) {
  assert(frame < GetNumFrames());
  current_frame_ = frame;
}

size_t ImageQuad::GetNumFrames() const {
  return num_frames_[0] * num_frames_[1];
}

void ImageQuad::Draw() {
  if (!IsVisible() || !texture_ || !texture_->IsValid())
    return;

  texture_->Activate();

  Vector2 tex_scale = {GetFrameWidth() / texture_->GetWidth(),
                       GetFrameHeight() / texture_->GetHeight()};

  std::shared_ptr<Geometry> quad = Engine::Get().GetQuad();
  std::shared_ptr<Shader> shader = Engine::Get().GetPassThroughShader();

  shader->Activate();
  shader->SetUniform("offset", offset_);
  shader->SetUniform("scale", scale_);
  shader->SetUniform("pivot", pivot_);
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("tex_offset", GetUVOffset(current_frame_));
  shader->SetUniform("tex_scale", tex_scale);
  shader->SetUniform("projection", Engine::Get().GetProjectionMarix());
  shader->SetUniform("color", color_);
  shader->SetUniform("texture", 0);

  quad->Draw();
}

float ImageQuad::GetFrameWidth() const {
  return frame_width_ > 0 ? (float)frame_width_
                          : texture_->GetWidth() / (float)num_frames_[0];
}

float ImageQuad::GetFrameHeight() const {
  return frame_height_ > 0 ? (float)frame_height_
                           : texture_->GetHeight() / (float)num_frames_[1];
}

// Return the uv offset for the given frame.
Vector2 ImageQuad::GetUVOffset(int frame) const {
  assert(frame < num_frames_[0] * num_frames_[1]);
  if (num_frames_[0] == 1 && num_frames_[1] == 1)
    return {0, 0};
  return {(float)(frame % num_frames_[0]), (float)(frame / num_frames_[0])};
}

}  // namespace eng
