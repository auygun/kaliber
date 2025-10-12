#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>

#include "base/vecmath.h"

class Frustum {
 public:
  base::Planef planes[6];

  static Frustum CreateFromMatrix(const base::Matrix4f& vp);

  // Intersection test methods
  bool Intersects(const base::AABBf& aabb) const;
  bool Intersects(const base::OBBf& oob, const base::Matrix4f& model) const;
};

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

std::unique_ptr<BVHNode> BuildBVHTree(std::vector<const MeshObject*>& objects);

std::vector<int> FrustumCull(const BVHNode* root, const Frustum& frustum);

void DumpBVHTree(const BVHNode* node,
                 const std::string& prefix,
                 bool isLast = true);

int TestBVH();

#endif  // BVH_H