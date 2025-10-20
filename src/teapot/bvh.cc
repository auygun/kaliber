#include "teapot/bvh.h"

#include <algorithm>
#include <deque>
#include <tuple>

#include "base/log.h"

using namespace base;

namespace {}  // namespace

void Plane::Translate(const base::Vector3f& v) {
  distance += normal.DotProduct(v);
}

void Plane::Transform(const base::Matrix4f& mat) {
  normal.MultiplyMatrix3x3(mat);
  normal.Normalize();
  Translate(mat.Row(3));
}

void AABB::Expand(const AABB& other) {
  min.x = std::min(min.x, other.min.x);
  min.y = std::min(min.y, other.min.y);
  min.z = std::min(min.z, other.min.z);
  max.x = std::max(max.x, other.max.x);
  max.y = std::max(max.y, other.max.y);
  max.z = std::max(max.z, other.max.z);
}

void AABB::Expand(const base::Vector3f& p) {
  min.x = std::min(min.x, p.x);
  min.y = std::min(min.y, p.y);
  min.z = std::min(min.z, p.z);
  max.x = std::max(max.x, p.x);
  max.y = std::max(max.y, p.y);
  max.z = std::max(max.z, p.z);
}

bool AABB::IsOutsidePlane(const Plane& p) const {
  Vector3f center{(max + min) * 0.5f};
  Vector3f extents{max - center};

  // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
  const float r = extents.x * std::abs(p.normal.x) +
                  extents.y * std::abs(p.normal.y) +
                  extents.z * std::abs(p.normal.z);

  float d = p.normal.DotProduct(center) - p.distance + r;
  return d < 0.0f;
}

void OBB::Transform(const base::Matrix4f& m) {
  for (int i = 0; i < 3; i++) {
    axes[i].MultiplyMatrix3x3(m);
    axes[i].Normalize();
  }
  center *= m;
}

void OBB::GetBoundBox(AABB& aabb) const {
  for (int k = 0; k < 3; k++)
    aabb.max.k[k] = std::abs(axes[0][k] * extents[0]) +
                    std::abs(axes[1][k] * extents[1]) +
                    std::abs(axes[2][k] * extents[2]);

  aabb.min = -aabb.max;
  aabb.min += center;
  aabb.max += center;
}

void OBB::GetLocalBox(AABB& aabb) const {
  aabb.min = center - extents;
  aabb.max = center + extents;
}

void Frustum::CreateFromMatrix(const Matrix4f& vp) {
  Vector4f raw_planes[6];
  raw_planes[0] = vp.Row4(3) + vp.Row4(0);
  raw_planes[1] = vp.Row4(3) - vp.Row4(0);
  raw_planes[2] = vp.Row4(3) + vp.Row4(1);
  raw_planes[3] = vp.Row4(3) - vp.Row4(1);
  raw_planes[4] = vp.Row4(2);
  raw_planes[5] = vp.Row4(3) - vp.Row4(2);

  for (int i = 0; i < 6; ++i) {
    Vector3f n = raw_planes[i].GetVector3();
    float d = raw_planes[i][3];
    float magnitude = n.Length();
    planes[i].normal = n / magnitude;
    planes[i].distance = -d / magnitude;
  }
}

void Frustum::CreateFromCamera(const Matrix4f& cam,
                               float aspect,
                               float fovY,
                               float zNear,
                               float zFar) {
  float fovRadians = fovY * (float)(M_PI / 180.0);
  const float halfVSide = zFar * tanf(fovRadians * .5f);
  const float halfHSide = halfVSide * aspect;
  const Vector3f frontMultFar = cam.Row(2) * zFar;

  // Near and Far planes
  planes[4] = {cam.Row(3) + cam.Row(2) * zNear, cam.Row(2)};
  planes[5] = {cam.Row(3) + frontMultFar, -cam.Row(2)};

  // Left and Right planes
  planes[0] = {cam.Row(3),
               cam.Row(0).CrossProduct(frontMultFar + cam.Row(1) * halfHSide)};
  planes[1] = {
      cam.Row(3),
      (frontMultFar - cam.Row(1) * halfHSide).CrossProduct(cam.Row(0))};

  // Top and Bottom planes
  planes[3] = {cam.Row(3),
               cam.Row(1).CrossProduct(frontMultFar - cam.Row(0) * halfVSide)};
  planes[2] = {
      cam.Row(3),
      (frontMultFar + cam.Row(0) * halfVSide).CrossProduct(cam.Row(1))};
}

// Test AABB vs. all 6 planes
bool Frustum::Intersects(const AABB& aabb) const {
  for (int i = 0; i < 6; ++i) {
    if (aabb.IsOutsidePlane(planes[i]))
      return false;
  }
  return true;
}

bool Frustum::Intersects(const OBB& obb, const Matrix4f& model) const {
  AABB aabb;
  obb.GetLocalBox(aabb);

  Matrix4f inverse_model;
  model.InverseOrthogonal(inverse_model);

  // Transform each plane to the model's local space and test
  for (int i = 0; i < 6; i++) {
    Plane p = planes[i];
    p.Transform(inverse_model);

    if (aabb.IsOutsidePlane(p))
      return false;
  }
  return true;
}

std::unique_ptr<BVHNode> BuildBVHTree(
    const std::vector<const MeshObject*>& objects) {
  if (objects.empty())
    return nullptr;

  auto root = std::make_unique<BVHNode>();

  // Create stack for depth-first traversal and start the process with the root
  // node using all objects.
  std::deque<std::tuple<BVHNode*, std::vector<const MeshObject*>>> stack;
  stack.push_back(std::make_tuple(root.get(), objects));

  while (!stack.empty()) {
    auto [node, node_objects] = stack.back();
    stack.pop_back();

    // If only one object remains, this is a leaf.
    if (node_objects.size() == 1) {
      node->object = node_objects[0];
      continue;
    }

    // Calculate the combined AABB for all node_objects in this job.
    for (const auto& obj : node_objects) {
      AABB aabb;
      obj->obb.GetBoundBox(aabb);
      node->aabb.Expand(aabb);
    }

    // Find the longest axis of the combined AABB to split along.
    Vector3f extent = node->aabb.max - node->aabb.min;
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;
    if (extent.z > extent.y)
      axis = 2;

    // Sort node_objects along the chosen axis based on their center point.
    if (axis == 0) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](const MeshObject* a, const MeshObject* b) {
                  return a->obb.center.x < b->obb.center.x;
                });
    } else if (axis == 1) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](const MeshObject* a, const MeshObject* b) {
                  return a->obb.center.y < b->obb.center.y;
                });
    } else {  // axis == 2
      std::sort(node_objects.begin(), node_objects.end(),
                [](const MeshObject* a, const MeshObject* b) {
                  return a->obb.center.z < b->obb.center.z;
                });
    }

    // Split the objects into two halves
    size_t mid = node_objects.size() / 2;
    std::vector<const MeshObject*> left_objects(node_objects.begin(),
                                                node_objects.begin() + mid);
    std::vector<const MeshObject*> right_objects(node_objects.begin() + mid,
                                                 node_objects.end());

    // Create the child nodes.
    node->left = std::make_unique<BVHNode>();
    node->right = std::make_unique<BVHNode>();

    // Push the children onto the stack.
    stack.push_back({node->right.get(), std::move(right_objects)});
    stack.push_back({node->left.get(), std::move(left_objects)});
  }

  return root;
}

std::vector<int> FrustumCull(const BVHNode* root, const Frustum& frustum) {
  std::vector<int> visible_object_ids;
  if (!root)
    return visible_object_ids;

  // Create stack for depth-first traversal and start the process with the root
  // of the BVH tree.
  std::deque<const BVHNode*> stack;
  stack.push_back(root);

  while (!stack.empty()) {
    const BVHNode* node = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (node->isLeaf()) {
      if (frustum.Intersects(node->object->obb, node->object->model))
        visible_object_ids.push_back(node->object->id);
      continue;
    } else if (!frustum.Intersects(node->aabb)) {
      continue;
    }

    // The internal node passed tests, check its children
    if (node->left)
      stack.push_back(node->left.get());
    if (node->right)
      stack.push_back(node->right.get());
  }

  return visible_object_ids;
}

void DumpBVHTree(const BVHNode* node, const std::string& prefix, bool is_last) {
  if (!node)
    return;

  std::ostringstream out;

  // Print the current node's line
  out << prefix;
  out << (is_last ? "└──" : "├──");

  AABB aabb;

  // Print node details
  if (node->object) {
    out << "[Leaf] ID: " << node->object->id << " ";
    node->object->obb.GetBoundBox(aabb);
  } else {
    out << "[Internal] ";
    aabb = node->aabb;
  }

  // Print bounding box info
  Vector3f center = (aabb.min + aabb.max) * 0.5f;
  float radius = (aabb.max - center).Length();
  out << "Center: (" << center.x << ", " << center.y << ", " << center.z
      << ") Radius: " << radius;
  DLOG(0) << out.str();

  // Prepare the prefix for the children
  std::string child_prefix = prefix + (is_last ? "    " : "│   ");

  // Recurse for children (if they exist)
  if (!node->object) {
    // The right child is always the "last" one for its parent
    DumpBVHTree(node->left.get(), child_prefix, false);
    DumpBVHTree(node->right.get(), child_prefix, true);
  }
}
