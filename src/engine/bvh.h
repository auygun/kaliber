#ifndef ENGINE_BVH_H
#define ENGINE_BVH_H

#include <memory>
#include <vector>

#include "base/vecmath.h"

namespace eng {

class DebugLayer;

// Represents a mesh object in the scene
struct MeshObject {
  size_t id = -1;
  base::OBBf obb;
  base::Matrix4f model{1};
  // In a real engine, this would point to the actual mesh data and transform
};

struct BVHNode {
  // Bounding volume for the node
  base::AABBf aabb;

  // Tree structure
  size_t left = (size_t)-1;
  size_t right = (size_t)-1;

  // size_t object_ind = (size_t)-1;  // If not -1, this is a leaf node
  MeshObject object;

  bool isLeaf() const { return object.id != (size_t)-1; }
};

std::vector<BVHNode> BuildBVHTree(std::vector<MeshObject> objects);

std::vector<size_t> FrustumCull(const std::vector<BVHNode>& nodes,
                             const base::Frustumf& frustum);

void DumpBVHTree(const std::vector<BVHNode>& nodes,
                 size_t node_ind,
                 const std::string& prefix,
                 bool is_last = true);

void DrawBVHTree(const std::vector<BVHNode>& nodes,
                 size_t node_ind,
                 DebugLayer& debug_layer);

}  // namespace eng

#endif  // ENGINE_BVH_H
