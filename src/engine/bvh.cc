#include "engine/bvh.h"

#include <algorithm>
#include <deque>
#include <tuple>

#include "base/log.h"

using namespace base;

namespace eng {

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
      AABBf aabb;
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

std::vector<int> FrustumCull(const BVHNode* root, const Frustumf& frustum) {
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
    } /*else if (!frustum.Intersects(node->aabb)) {
      continue;
    }*/

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

  AABBf aabb;

  // Print node details
  if (node->object) {
    out << "[Leaf] ID: " << node->object->id << " ";
    node->object->obb.GetBoundBox(aabb);
  } else {
    out << "[Internal] ";
    aabb = node->aabb;
  }

  // Print bounding box info
  Vector3f center = (aabb.max + aabb.min) * 0.5f;
  Vector3f extent = (aabb.max - aabb.min) * 0.5f;
  out << "Center: " << center.ToString() << " Extent: " << extent.ToString();
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

}  // namespace eng
