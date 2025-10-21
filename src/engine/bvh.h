#ifndef ENGINE_BVH_H
#define ENGINE_BVH_H

#include <memory>
#include <vector>

#include "base/vecmath.h"

namespace eng {

// Represents a mesh object in the scene
struct MeshObject {
  int id;
  base::OBBf obb;
  base::Matrix4f model{1};
  // In a real engine, this would point to the actual mesh data and transform
};

struct BVHNode {
  // Bounding volume for the node
  base::AABBf aabb;

  // Tree structure
  std::unique_ptr<BVHNode> left = nullptr;
  std::unique_ptr<BVHNode> right = nullptr;

  // Payload: only one is valid
  const MeshObject* object = nullptr;  // If not null, this is a leaf node

  bool isLeaf() const { return object != nullptr; }
};

std::unique_ptr<BVHNode> BuildBVHTree(
    const std::vector<const MeshObject*>& objects);

std::vector<int> FrustumCull(const BVHNode* root, const base::Frustumf& frustum);

void DumpBVHTree(const BVHNode* node,
                 const std::string& prefix,
                 bool is_last = true);

}  // namespace eng

#endif  // ENGINE_BVH_H
