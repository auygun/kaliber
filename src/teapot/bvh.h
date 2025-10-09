#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "base/vecmath.h"

// --- Core Geometric Primitives ---

struct Vec3 {
  float x = 0.0f, y = 0.0f, z = 0.0f;
  // Basic vector operations can be added here (dot, cross, etc.)
};

struct BoundingSphere {
  Vec3 center = {0, 0, 0};
  float radius = 0.0f;
};

struct AABB {  // Axis-Aligned Bounding Box
  Vec3 min = {std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max()};
  Vec3 max = {-std::numeric_limits<float>::max(),
              -std::numeric_limits<float>::max(),
              -std::numeric_limits<float>::max()};

  void expand(const AABB& other) {
    min.x = std::min(min.x, other.min.x);
    min.y = std::min(min.y, other.min.y);
    min.z = std::min(min.z, other.min.z);
    max.x = std::max(max.x, other.max.x);
    max.y = std::max(max.y, other.max.y);
    max.z = std::max(max.z, other.max.z);
  }
};

struct OBB {  // Oriented Bounding Box
  Vec3 center = {0, 0, 0};
  Vec3 extents = {0, 0, 0};  // Half-sizes along each axis
  Vec3 axes[3] = {{1, 0, 0},
                  {0, 1, 0},
                  {0, 0, 1}};  // Orientation (rotation matrix columns)
};

// Represents a mesh object in the scene
struct MeshObject {
  int id;
  AABB initialAABB;  // Used for calculating OBB and initial sphere
  // In a real engine, this would point to the actual mesh data and transform
};

// --- Frustum for Culling ---
// A frustum is defined by 6 planes
struct Plane {
  Vec3 normal = {0, 1, 0};
  float distance = 0.0f;  // Distance from origin
};

class Frustum {
 public:
  Plane planes[6];

  Frustum createFromMatrix(const base::Matrix4f& vp);

  // Intersection test methods
  bool intersects(const BoundingSphere& sphere) const;
  bool intersects(const AABB& aabb) const;
  bool intersects(const OBB& obb) const;
};

// --- BVH Node Structure ---

struct BVHNode {
  // Bounding volumes for the node
  BoundingSphere sphere;
  AABB aabb;
  OBB obb;  // Only valid for leaf nodes

  // Tree structure
  std::unique_ptr<BVHNode> left = nullptr;
  std::unique_ptr<BVHNode> right = nullptr;

  // Payload: only one is valid
  const MeshObject* object = nullptr;  // If not null, this is a leaf node

  bool isLeaf() const { return object != nullptr; }
};

// --- The BVH Class ---

class BVH {
 public:
  BVH() = default;

  // Rebuilds the BVH from the provided list of objects
  void build(const std::vector<MeshObject>& objects);

  // Performs frustum culling and returns a list of visible object IDs
  std::vector<int> frustumCull(const Frustum& frustum) const;

  void dumpTree() const;

 private:
  std::unique_ptr<BVHNode> root = nullptr;

  // Recursive function to build the tree
  std::unique_ptr<BVHNode> buildRecursive(
      std::vector<const MeshObject*>& objects);

  // Recursive function for frustum culling
  void frustumCullRecursive(const BVHNode* node,
                            const Frustum& frustum,
                            std::vector<int>& visibleObjectIDs) const;

  void dumpNodeRecursive(const BVHNode* node,
                         const std::string& prefix,
                         bool isLast) const;
};

int TestBVH();

#endif  // BVH_H