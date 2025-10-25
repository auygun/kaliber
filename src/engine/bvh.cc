#include "engine/bvh.h"

#include <algorithm>
#include <deque>
#include <span>
#include <tuple>

#include "base/log.h"
#include "engine/debug_layer.h"

using namespace base;

namespace eng {

std::vector<BVHNode> BuildBVHTree(std::vector<WorldObject> objects) {
  if (objects.empty())
    return {};

  std::vector<BVHNode> bvh_nodes(2 * objects.size() - 1);
  size_t node_ind_last = 0;  // std::make_unique<BVHNode>();

  // Create stack for depth-first traversal and start the process with the root
  // node using all objects.
  std::deque<std::tuple<size_t, std::span<WorldObject>>> stack;
  std::span objects_view(objects.data(), objects.size());
  stack.push_back(std::make_tuple(node_ind_last, std::move(objects_view)));

  while (!stack.empty()) {
    auto [node_ind, node_objects] = std::move(stack.back());
    stack.pop_back();

    // If only one object remains, this is a leaf.
    if (node_objects.size() == 1) {
      bvh_nodes[node_ind].object = node_objects[0];
      continue;
    }

    // Calculate the combined AABB for all node_objects in this job.
    for (auto& obj : node_objects) {
      AABBf aabb;
      obj.obb.GetBoundBox(aabb);
      bvh_nodes[node_ind].aabb.Expand(aabb);
    }

    // Find the longest axis of the combined AABB to split along.
    Vector3f extent =
        bvh_nodes[node_ind].aabb.max - bvh_nodes[node_ind].aabb.min;
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;
    if (extent.z > extent.y)
      axis = 2;

    // Sort node_objects along the chosen axis based on their center point.
    if (axis == 0) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.x < b.obb.center.x;
                });
    } else if (axis == 1) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.y < b.obb.center.y;
                });
    } else {  // axis == 2
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.z < b.obb.center.z;
                });
    }

    // Split the objects into two halves
    size_t mid = node_objects.size() / 2;
    std::span<WorldObject> left_objects(node_objects.begin(),
                                        node_objects.begin() + mid);
    std::span<WorldObject> right_objects(node_objects.begin() + mid,
                                         node_objects.end());

    // Create the child nodes.
    bvh_nodes[node_ind].left = ++node_ind_last;
    bvh_nodes[node_ind].right = ++node_ind_last;

    // Push the children onto the stack.
    stack.push_back({bvh_nodes[node_ind].left, std::move(right_objects)});
    stack.push_back({bvh_nodes[node_ind].right, std::move(left_objects)});
  }

  return bvh_nodes;
}

std::vector<size_t> FrustumCull(const std::vector<BVHNode>& nodes,
                                const Frustumf& frustum) {
  std::vector<size_t> visible_object_ids;
  if (nodes.empty())
    return visible_object_ids;

  // Create stack for depth-first traversal and start the process with the root
  // of the BVH tree.
  std::deque<size_t> stack;
  stack.push_back(0);

  while (!stack.empty()) {
    size_t node_ind = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (nodes[node_ind].isLeaf()) {
      if (frustum.Intersects(nodes[node_ind].object.obb,
                             nodes[node_ind].object.transform))
        visible_object_ids.push_back(nodes[node_ind].object.id);
      continue;
    } else if (!frustum.Intersects(nodes[node_ind].aabb)) {
      continue;
    }

    // The internal node passed tests, check its children
    if (nodes[node_ind].left)
      stack.push_back(nodes[node_ind].left);
    if (nodes[node_ind].right)
      stack.push_back(nodes[node_ind].right);
  }

  return visible_object_ids;
}

void DumpBVHTree(const std::vector<BVHNode>& nodes,
                 size_t node_ind,
                 const std::string& prefix,
                 bool is_last) {
  if (nodes.empty())
    return;

  std::ostringstream out;

  // Print the current node's line
  out << prefix;
  out << (is_last ? "└──" : "├──");

  AABBf aabb;

  // Print node details
  if (nodes[node_ind].object.id != (size_t)-1) {
    out << "[Leaf] ID: " << nodes[node_ind].object.id << " ";
    nodes[node_ind].object.obb.GetBoundBox(aabb);
  } else {
    out << "[Internal] ";
    aabb = nodes[node_ind].aabb;
  }

  // Print bounding box info
  Vector3f center = (aabb.max + aabb.min) * 0.5f;
  Vector3f extent = (aabb.max - aabb.min) * 0.5f;
  out << "Center: " << center.ToString() << " Extent: " << extent.ToString();
  DLOG(0) << out.str();

  // Prepare the prefix for the children
  std::string child_prefix = prefix + (is_last ? "    " : "│   ");

  // Recurse for children (if they exist)
  if (nodes[node_ind].object.id == (size_t)-1) {
    // The right child is always the "last" one for its parent
    DumpBVHTree(nodes, nodes[node_ind].left, child_prefix, false);
    DumpBVHTree(nodes, nodes[node_ind].right, child_prefix, true);
  }
}

void DrawBVHTree(const std::vector<BVHNode>& nodes,
                 size_t node_ind,
                 DebugLayer& debug_layer) {
  if (nodes.empty())
    return;

  if (nodes[node_ind].object.id != (size_t)-1) {
    debug_layer.DrawObb(nodes[node_ind].object.obb, {1, 1, 0});
  } else {
    debug_layer.DrawAabb(nodes[node_ind].aabb, {1, 0, 1});
  }

  // Recurse for children (if they exist)
  if (nodes[node_ind].object.id == (size_t)-1) {
    // The right child is always the "last" one for its parent
    DrawBVHTree(nodes, nodes[node_ind].left, debug_layer);
    DrawBVHTree(nodes, nodes[node_ind].right, debug_layer);
  }
}

}  // namespace eng
