#include "image_quad.h"
#include "../base/log.h"
#include "../engine/image.h"
#include "../engine/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include <cassert>

using base::Vector2;

namespace eng {

void ImageQuad::Create(std::shared_ptr<const Image> image,
                       std::array<int, 2> num_frames,
                       int frame_width,
                       int frame_height) {
  assert(num_frames_[0] > 0 && num_frames_[1] > 0);
  assert(image->IsImmutable());

  texture_.Update(image);

  if (frame_width > 0)
    frame_width_ = frame_width;
  else
    frame_width_ = image->GetOriginalWidth() / num_frames[0];

  if (frame_height > 0)
    frame_height_ = frame_height;
  else
    frame_height_ = image->GetOriginalHeight() / num_frames[1];

  tex_scale_ = {
    (float)frame_width_ / (float)image->GetWidth(),
    (float)frame_height_ / (float)image->GetHeight()
  };
  num_frames_ = std::move(num_frames);
}

void ImageQuad::AutoScale() {
  SetScale(Engine::Get().ToScale(Vector2(frame_width_, frame_height_)));
  Scale((float)Engine::Get().GetDeviceDpi() / 200.0f);
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
