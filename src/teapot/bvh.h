#ifndef BVH_H
#define BVH_H

#include <limits>
#include <memory>
#include <vector>

#include "base/vecmath.h"

// The plane equation is: normal · point - distance = 0
struct Plane {
  base::Vector3f normal;
  float distance;

  Plane() = default;

  Plane(const base::Vector3f& n, float d) : normal{n}, distance{d} {}

  Plane(const base::Vector3f& p, const base::Vector3f& n) : normal{n} {
    normal.Normalize();
    distance = -(normal.DotProduct(p));
  }

  void Translate(const base::Vector3f& v);
  void Transform(const base::Matrix4f& mat);
};

// Axis-Aligned Bounding Box
struct AABB {
  base::Vector3f min{std::numeric_limits<float>::max()};
  base::Vector3f max{std::numeric_limits<float>::lowest()};

  void Expand(const AABB& other);
  void Expand(const base::Vector3f& p);

  bool IsOutsidePlane(const Plane& p) const;
};

// Oriented Bounding Box
struct OBB {
  base::Vector3f center{0, 0, 0};
  base::Vector3f extents{0, 0, 0};  // Half-sizes along each axis
  base::Vector3f axes[3]{{1, 0, 0},
                         {0, 1, 0},
                         {0, 0, 1}};  // Orientation (rotation matrix columns)

  // Computes the World-Space AABB that tightly encloses this OBB.
  void GetBoundBox(AABB& aabb) const;
};

class Frustum {
 public:
  Plane planes[6];

  void CreateFromMatrix(const base::Matrix4f& vp);

  void CreateFromCamera(const base::Matrix4f& cam,
                        float aspect,
                        float fovY,
                        float zNear,
                        float zFar);

  // Intersection test methods
  bool Intersects(const AABB& aabb) const;
  bool Intersects(const OBB& oob, const base::Matrix4f& model) const;
};

// Represents a mesh object in the scene
struct MeshObject {
  int id;
  OBB obb;
  base::Matrix4f model{1};
  // In a real engine, this would point to the actual mesh data and transform
};

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

std::unique_ptr<BVHNode> BuildBVHTree(
    const std::vector<const MeshObject*>& objects);

std::vector<int> FrustumCull(const BVHNode* root, const Frustum& frustum);

void DumpBVHTree(const BVHNode* node,
                 const std::string& prefix,
                 bool is_last = true);

int TestBVH();

#endif  // BVH_H