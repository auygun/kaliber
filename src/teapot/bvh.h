#ifndef BVH_H
#define BVH_H

#include <limits>
#include <memory>
#include <vector>

#include "base/vecmath.h"

// Represents a mesh object in the scene
struct MeshObject {
  int id;
  base::OBB obb;
  base::Matrix4f model{1};
  // In a real engine, this would point to the actual mesh data and transform
};

struct BVHNode {
  // Bounding volume for the node
  base::AABB aabb;

  // Tree structure
  std::unique_ptr<BVHNode> left = nullptr;
  std::unique_ptr<BVHNode> right = nullptr;

  // Payload: only one is valid
  const MeshObject* object = nullptr;  // If not null, this is a leaf node

  bool isLeaf() const { return object != nullptr; }
};

std::unique_ptr<BVHNode> BuildBVHTree(
    const std::vector<const MeshObject*>& objects);

std::vector<int> FrustumCull(const BVHNode* root, const base::Frustum& frustum);

void DumpBVHTree(const BVHNode* node,
                 const std::string& prefix,
                 bool is_last = true);

#endif  // BVH_H
