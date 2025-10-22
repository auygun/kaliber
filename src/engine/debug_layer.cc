#include "engine/debug_layer.h"

#include <algorithm>
#include <cmath>

#include "engine/asset/shader_source.h"
#include "engine/renderer/renderer.h"

using namespace base;

namespace eng {

namespace {

const char vertex_description[] = "p3f;c4f";

struct PushConstant {
  Matrix4f view_projection;
};

}  // namespace

// --- Helper function for 3-plane intersection ---
// Solves a 3x3 system of linear equations using Cramer's rule.
// Finds the intersection point of three planes.
static Vector3f planeIntersection(const Planef& p1,
                                  const Planef& p2,
                                  const Planef& p3) {
  Vector3f n1 = p1.normal;
  Vector3f n2 = p2.normal;
  Vector3f n3 = p3.normal;

  float det = n1.DotProduct(n2.CrossProduct(n3));

  if (std::abs(det) < 1e-6f) {
    // No unique intersection point (planes are parallel or coincident)
    return Vector3f(0.0f);
  }

  // p = ( d1(n2 x n3) + d2(n3 x n1) + d3(n1 x n2) ) / det
  return (n2.CrossProduct(n3) * p1.distance +
          n3.CrossProduct(n1) * p2.distance +
          n1.CrossProduct(n2) * p3.distance) /
         det;
}

DebugLayer::DebugLayer() {
  if (!ParseVertexDescription(vertex_description, vertex_description_))
    LOG(0) << "Failed to parse vertex description.";
}

void DebugLayer::CreateRenderResources(Renderer* renderer) {
  renderer_ = renderer;
  shader_.SetRenderer(renderer);

  // Create the shader.
  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/debug.glsl")) {
    shader_.Create(std::move(source), vertex_description_, kPrimitive_Lines,
                   false, true, CullMode::kNone);
  } else {
    LOG(0) << "Could not create debug shader.";
  }

  geometry_.SetRenderer(renderer);
  geometry_.Create(kPrimitive_Lines, vertex_description_, kDataType_Invalid);
}

void DebugLayer::Update(float delta_time) {
  for (auto& shape : shapes_) {
    shape.remaining_time -= delta_time;
    if (shape.fade && shape.initial_duration > 0.0f) {
      float alpha =
          std::clamp(shape.remaining_time / shape.initial_duration, 0.0f, 1.0f);
      for (auto& vertex : shape.vertices) {
        vertex.color.k[3] = alpha;
      }
    }
  }

  // Remove expired shapes
  shapes_.erase(std::remove_if(shapes_.begin(), shapes_.end(),
                               [](const DebugShape& shape) {
                                 return shape.remaining_time <= 0;
                               }),
                shapes_.end());
}

void DebugLayer::Draw(const Matrix4f& view_projection) {
  // Collect all vertices from all active shapes
  aggregated_vertices_.clear();
  for (const auto& shape : shapes_) {
    aggregated_vertices_.insert(aggregated_vertices_.end(),
                                shape.vertices.begin(), shape.vertices.end());
  }

  if (aggregated_vertices_.empty())
    return;

  geometry_.Update(aggregated_vertices_.size(), aggregated_vertices_.data(), 0,
                   nullptr);

  shader_.Activate();
  renderer_->ActivateGeometry(geometry_.resource_id());
  PushConstant pc{};
  pc.view_projection = view_projection;
  renderer_->UpdatePushConstants(sizeof(pc), &pc);
  geometry_.Draw(0, 0);
}

void DebugLayer::Clear() {
  shapes_.clear();
}

void DebugLayer::DrawLine(const Vector3f& start,
                          const Vector3f& end,
                          const Vector3f& color,
                          float duration,
                          bool fade) {
  DebugShape shape;
  shape.vertices.push_back({start, Vector4f(color, 1.0f)});
  shape.vertices.push_back({end, Vector4f(color, 1.0f)});
  shape.remaining_time = duration;
  shape.initial_duration =
      duration > 0.0f ? duration : 1.0f;  // Avoid division by zero
  shape.fade = fade;
  shapes_.push_back(shape);
}

void DebugLayer::DrawVector(const Vector3f& start,
                            const Vector3f& vector,
                            const Vector3f& color,
                            float duration,
                            bool fade) {
  Vector3f end = start + vector;
  DrawLine(start, end, color, duration, fade);

  // Arrowhead
  float length = vector.Length();
  if (length > 0.001f) {
    Vector3f dir = vector;
    dir.Normalize();
    Vector3f right = dir.CrossProduct(Vector3f(0, 1, 0));
    if (right.Length() < 0.1f) {
      right = dir.CrossProduct(Vector3f(1, 0, 0));
    }
    right.Normalize();
    Vector3f up = dir.CrossProduct(right);

    float arrowhead_size = length * 0.1f;
    Vector3f arrowhead_base = end - dir * arrowhead_size;

    DrawLine(end, arrowhead_base + right * arrowhead_size * 0.5f, color,
             duration, fade);
    DrawLine(end, arrowhead_base - right * arrowhead_size * 0.5f, color,
             duration, fade);
    DrawLine(end, arrowhead_base + up * arrowhead_size * 0.5f, color, duration,
             fade);
    DrawLine(end, arrowhead_base - up * arrowhead_size * 0.5f, color, duration,
             fade);
  }
}

void DebugLayer::DrawMatrix(const Matrix4f& matrix,
                            float axisLength,
                            float duration,
                            bool fade) {
  Vector3f position = matrix.Row(3);
  Vector3f xAxis = matrix.Row(0) * axisLength;
  Vector3f yAxis = matrix.Row(1) * axisLength;
  Vector3f zAxis = matrix.Row(2) * axisLength;

  DrawVector(position, xAxis, Vector3f(1.0f, 0.0f, 0.0f), duration,
             fade);  // Red X
  DrawVector(position, yAxis, Vector3f(0.0f, 1.0f, 0.0f), duration,
             fade);  // Green Y
  DrawVector(position, zAxis, Vector3f(0.0f, 0.0f, 1.0f), duration,
             fade);  // Blue Z
}

void DebugLayer::DrawQuaternion(const Vector3f& position,
                                const Quatf& orientation,
                                float axisLength,
                                float duration,
                                bool fade) {
  Matrix4f rotation_matrix;
  orientation.CreateMatrix(rotation_matrix);
  DrawMatrix(rotation_matrix, axisLength, duration, fade);
}

void DebugLayer::DrawAabb(const AABBf& aabb,
                          const Vector3f& color,
                          float duration,
                          bool fade) {
  Vector3f corners[8];
  corners[0] = Vector3f(aabb.min.x, aabb.min.y, aabb.min.z);
  corners[1] = Vector3f(aabb.max.x, aabb.min.y, aabb.min.z);
  corners[2] = Vector3f(aabb.max.x, aabb.max.y, aabb.min.z);
  corners[3] = Vector3f(aabb.min.x, aabb.max.y, aabb.min.z);
  corners[4] = Vector3f(aabb.min.x, aabb.min.y, aabb.max.z);
  corners[5] = Vector3f(aabb.max.x, aabb.min.y, aabb.max.z);
  corners[6] = Vector3f(aabb.max.x, aabb.max.y, aabb.max.z);
  corners[7] = Vector3f(aabb.min.x, aabb.max.y, aabb.max.z);

  // Draw the 12 edges
  DrawLine(corners[0], corners[1], color, duration, fade);
  DrawLine(corners[1], corners[2], color, duration, fade);
  DrawLine(corners[2], corners[3], color, duration, fade);
  DrawLine(corners[3], corners[0], color, duration, fade);
  DrawLine(corners[4], corners[5], color, duration, fade);
  DrawLine(corners[5], corners[6], color, duration, fade);
  DrawLine(corners[6], corners[7], color, duration, fade);
  DrawLine(corners[7], corners[4], color, duration, fade);
  DrawLine(corners[0], corners[4], color, duration, fade);
  DrawLine(corners[1], corners[5], color, duration, fade);
  DrawLine(corners[2], corners[6], color, duration, fade);
  DrawLine(corners[3], corners[7], color, duration, fade);
}

void DebugLayer::DrawObb(const OBBf& obb,
                         const Vector3f& color,
                         float duration,
                         bool fade) {
  Vector3f ext = obb.extents;
  Matrix4f orientation{1};
  orientation.Row(0) = obb.axes[0];
  orientation.Row(1) = obb.axes[1];
  orientation.Row(2) = obb.axes[2];

  Vector3f corners[8];
  corners[0] = obb.center + Vector3f(-ext.x, -ext.y, -ext.z) * orientation;
  corners[1] = obb.center + Vector3f(ext.x, -ext.y, -ext.z) * orientation;
  corners[2] = obb.center + Vector3f(ext.x, ext.y, -ext.z) * orientation;
  corners[3] = obb.center + Vector3f(-ext.x, ext.y, -ext.z) * orientation;
  corners[4] = obb.center + Vector3f(-ext.x, -ext.y, ext.z) * orientation;
  corners[5] = obb.center + Vector3f(ext.x, -ext.y, ext.z) * orientation;
  corners[6] = obb.center + Vector3f(ext.x, ext.y, ext.z) * orientation;
  corners[7] = obb.center + Vector3f(-ext.x, ext.y, ext.z) * orientation;

  // Draw the 12 edges
  DrawLine(corners[0], corners[1], color, duration, fade);
  DrawLine(corners[1], corners[2], color, duration, fade);
  DrawLine(corners[2], corners[3], color, duration, fade);
  DrawLine(corners[3], corners[0], color, duration, fade);
  DrawLine(corners[4], corners[5], color, duration, fade);
  DrawLine(corners[5], corners[6], color, duration, fade);
  DrawLine(corners[6], corners[7], color, duration, fade);
  DrawLine(corners[7], corners[4], color, duration, fade);
  DrawLine(corners[0], corners[4], color, duration, fade);
  DrawLine(corners[1], corners[5], color, duration, fade);
  DrawLine(corners[2], corners[6], color, duration, fade);
  DrawLine(corners[3], corners[7], color, duration, fade);
}

void DebugLayer::DrawFrustum(const Planef frustumPlanes[6],
                             const Vector3f& color,
                             float duration,
                             bool fade) {
  // Assumes plane order: [0]=Left, [1]=Right, [2]=Bottom, [3]=Top, [4]=Near,
  // [5]=Far
  const Planef& left = frustumPlanes[0];
  const Planef& right = frustumPlanes[1];
  const Planef& bottom = frustumPlanes[2];
  const Planef& top = frustumPlanes[3];
  const Planef& near = frustumPlanes[4];
  const Planef& far = frustumPlanes[5];

  Vector3f corners[8];
  // Near face (Top-Left, Top-Right, Bottom-Right, Bottom-Left)
  corners[0] = planeIntersection(near, left, bottom);
  corners[1] = planeIntersection(near, right, bottom);
  corners[2] = planeIntersection(near, right, top);
  corners[3] = planeIntersection(near, left, top);
  // Far face
  corners[4] = planeIntersection(far, left, bottom);
  corners[5] = planeIntersection(far, right, bottom);
  corners[6] = planeIntersection(far, right, top);
  corners[7] = planeIntersection(far, left, top);

  // Draw near face
  DrawLine(corners[0], corners[1], color, duration, fade);
  DrawLine(corners[1], corners[2], color, duration, fade);
  DrawLine(corners[2], corners[3], color, duration, fade);
  DrawLine(corners[3], corners[0], color, duration, fade);

  // Draw far face
  DrawLine(corners[4], corners[5], color, duration, fade);
  DrawLine(corners[5], corners[6], color, duration, fade);
  DrawLine(corners[6], corners[7], color, duration, fade);
  DrawLine(corners[7], corners[4], color, duration, fade);

  // Draw connecting edges
  DrawLine(corners[0], corners[4], color, duration, fade);
  DrawLine(corners[1], corners[5], color, duration, fade);
  DrawLine(corners[2], corners[6], color, duration, fade);
  DrawLine(corners[3], corners[7], color, duration, fade);
}

}  // namespace eng
