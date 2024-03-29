#include "engine/image_quad.h"

#include "base/log.h"
#include "engine/engine.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

using namespace base;

namespace eng {

ImageQuad::ImageQuad() = default;

ImageQuad::~ImageQuad() {
  Destroy();
}

ImageQuad& ImageQuad::Create(const std::string& asset_name,
                             std::array<int, 2> num_frames,
                             int frame_width,
                             int frame_height) {
  texture_ = Engine::Get().AcquireTexture(asset_name);
  num_frames_ = std::move(num_frames);
  frame_width_ = frame_width;
  frame_height_ = frame_height;
  asset_name_ = asset_name;

  DCHECK((frame_width_ > 0 && frame_height_ > 0) || texture_->IsValid())
      << asset_name;
  SetSize(Engine::Get().ToViewportScale({GetFrameWidth(), GetFrameHeight()}) *
          Engine::Get().GetImageScaleFactor());
  return *this;
}

void ImageQuad::Destroy() {
  texture_.reset();
}

void ImageQuad::SetFrame(size_t frame) {
  DCHECK(frame < GetNumFrames())
      << "asset: " << asset_name_ << " frame: " << frame;
  current_frame_ = frame;
}

size_t ImageQuad::GetNumFrames() const {
  return num_frames_[0] * num_frames_[1];
}

void ImageQuad::Draw(float frame_frac) {
  DCHECK(IsVisible());

  if (!texture_ || !texture_->IsValid())
    return;

  texture_->Activate(0);

  Vector2f tex_scale = {GetFrameWidth() / texture_->GetWidth(),
                        GetFrameHeight() / texture_->GetHeight()};

  Shader* shader = GetCustomShader().get();
  if (!shader)
    shader = &Engine::Get().GetPassThroughShader();

  shader->Activate();
  shader->SetUniform("offset", position_);
  shader->SetUniform("scale", GetSize());
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("tex_offset", GetUVOffset(current_frame_));
  shader->SetUniform("tex_scale", tex_scale);
  shader->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader->SetUniform("color", color_);
  shader->SetUniform("texture_0", 0);
  DoSetCustomUniforms();
  Engine::Get().GetQuad().Draw();
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
Vector2f ImageQuad::GetUVOffset(size_t frame) const {
  DCHECK(frame < GetNumFrames())
      << "asset: " << asset_name_ << " frame: " << frame;
  return {(float)(frame % num_frames_[0]), (float)(frame / num_frames_[0])};
}

}  // namespace eng
