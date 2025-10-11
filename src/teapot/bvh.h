#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "base/vecmath.h"

// --- Core Geometric Primitives ---

struct OBB {  // Oriented Bounding Box
  base::Vector3f center{0, 0, 0};
  base::Vector3f extents{0, 0, 0};  // Half-sizes along each axis
  base::Vector3f axes[3]{{1, 0, 0},
                         {0, 1, 0},
                         {0, 0, 1}};  // Orientation (rotation matrix columns)
};

struct AABB {  // Axis-Aligned Bounding Box
  base::Vector3f min{std::numeric_limits<float>::max()};
  base::Vector3f max{std::numeric_limits<float>::lowest()};

  // Computes the World-Space AABB that tightly encloses the given OBB.
  static AABB CreateFromOBB(const OBB& obb);

  void expand(const AABB& other) {
    min.x = std::min(min.x, other.min.x);
    min.y = std::min(min.y, other.min.y);
    min.z = std::min(min.z, other.min.z);
    max.x = std::max(max.x, other.max.x);
    max.y = std::max(max.y, other.max.y);
    max.z = std::max(max.z, other.max.z);
  }

  void expand(const base::Vector3f& p) {
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
  }
};

// Represents a mesh object in the scene
struct MeshObject {
  int id;
  OBB obb;
  base::Matrix4f model{1};
  // In a real engine, this would point to the actual mesh data and transform
};

// --- Frustum for Culling ---
// The plane equation is: normal · point - distance = 0
struct Plane {
  base::Vector3f normal{0, 1, 0};
  float distance = 0.0f;  // Signed distance from origin along the normal

  void Translate(const base::Vector3f& v);

  void Transform(const base::Matrix4f& mat);

  bool IsOutside(const AABB& aabb) const;
};

class Frustum {
 public:
  Plane planes[6];

  Frustum CreateFromMatrix(const base::Matrix4f& vp);

  // Intersection test methods
  bool Intersects(const AABB& aabb) const;
  bool Intersects(const OBB& oob, const base::Matrix4f& model) const;
};

// --- BVH Node Structure ---

struct BVHNode {
  // Bounding volume for the node
  AABB aabb;

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

  std::unique_ptr<BVHNode> buildIterative(
      std::vector<const MeshObject*>& objects);

  // Recursive function for frustum culling
  void frustumCullRecursive(const BVHNode* node,
                            const Frustum& frustum,
                            std::vector<int>& visibleObjectIDs) const;

  std::vector<int> frustumCullIterative(const BVHNode* root,
                                        const Frustum& frustum) const;

  void dumpNodeRecursive(const BVHNode* node,
                         const std::string& prefix,
                         bool isLast) const;
};

int TestBVH();

#endif  // BVH_H