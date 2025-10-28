#ifndef ENGINE_DEBUG_LAYER_H
#define ENGINE_DEBUG_LAYER_H

#include <vector>

#include "base/vecmath.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/shader.h"

namespace eng {

class Renderer;

class DebugLayer {
 public:
  DebugLayer();
  ~DebugLayer() = default;

  // Initializes the debug layer, creating shaders and buffers.
  void CreateRenderResources(Renderer* renderer);

  // Updates the lifetime and fade of all shapes. Call this once per frame.
  void Update(float delta_time);

  // Call this at the end of a frame to render all debug shapes.
  void Draw(const base::Matrix4f& view_projection);

  // Clears all shapes immediately.
  void Clear();

  // --- Drawing Interface ---
  // duration: Time in seconds for the shape to be visible. 0 or less means one
  // frame. fade: If true, the shape will fade out over its duration.

  // Draws a line segment between two points.
  void DrawLine(const base::Vector3f& start,
                const base::Vector3f& end,
                const base::Vector3f& color = base::Vector3f{1.0f},
                float duration = 0.0f,
                bool fade = false);

  // Draws a vector as an arrow from a starting point.
  void DrawVector(const base::Vector3f& start,
                  const base::Vector3f& vector,
                  const base::Vector3f& color = base::Vector3f{1.0f},
                  float duration = 0.0f,
                  bool fade = false);

  // Renders a 4x4 matrix's orientation and position.
  // X-axis (red), Y-axis (green), Z-axis (blue).
  void DrawMatrix(const base::Matrix4f& matrix,
                  float axisLength = 1.0f,
                  float duration = 0.0f,
                  bool fade = false);

  // Renders a quaternion's orientation at a specific position.
  // X-axis (red), Y-axis (green), Z-axis (blue).
  void DrawQuaternion(const base::Vector3f& position,
                      const base::Quatf& orientation,
                      float axisLength = 1.0f,
                      float duration = 0.0f,
                      bool fade = false);

  // Draws an Axis-Aligned Bounding Box (AABB).
  void DrawAabb(const base::AABBf& aabb,
                const base::Vector3f& color = base::Vector3f{1.0f},
                float duration = 0.0f,
                bool fade = false);

  // Draws an Oriented Bounding Box (OBB).
  void DrawObb(const base::OBBf& obb,
               const base::Vector3f& color = base::Vector3f{1.0f},
               float duration = 0.0f,
               bool fade = false);

  // Draws a frustum from its 6 defining planes.
  void DrawFrustum(const base::Frustumf& frustum,
                   const base::Vector3f& color = base::Vector3f{1.0f},
                   float duration = 0.0f,
                   bool fade = false);

 private:
  // Struct to hold vertex data. Color now includes alpha.
  struct DebugVertex {
    base::Vector3f position;
    base::Vector4f color;
  };

  // Struct to manage an individual shape's lifetime and vertices.
  struct DebugShape {
    std::vector<DebugVertex> vertices;
    float remaining_time;
    float initial_duration;
    bool fade;
  };

  std::vector<DebugShape> shapes_;
  std::vector<DebugVertex> aggregated_vertices_;

  Renderer* renderer_ = nullptr;
  VertexDescription vertex_description_;
  Geometry geometry_;
  Shader shader_;
};

}  // namespace eng

#endif  // ENGINE_DEBUG_LAYER_H
