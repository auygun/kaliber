#include "teapot/bvh.h"

#include <algorithm>
#include <deque>
#include <tuple>

#include "base/log.h"

using namespace base;

namespace {

// Helper function to create some random mesh objects for testing
std::vector<MeshObject> CreateTestObjects(int count) {
  std::vector<MeshObject> objects;
  for (int i = 0; i < count; ++i) {
    MeshObject obj;
    obj.id = i;
    float x = static_cast<float>(rand() % 200 - 100);
    float y = static_cast<float>(rand() % 200 - 100);
    float z = static_cast<float>(rand() % 200 - 100);
    float size = static_cast<float>(rand() % 5 + 1);

    obj.obb.center = {0, 0, 0};
    obj.obb.extents = {size, size, size};
    obj.model.CreateTranslation({x, y, z});
    objects.push_back(obj);
  }
  return objects;
}

// Helper to create a sample frustum for testing
Frustum CreateTestFrustum() {
  Frustum f;
  // This is a simplified frustum. A real one would be derived from a projection
  // matrix. For this example, we define 6 planes that form a box-like view
  // volume.
  f.planes[0] = {{1, 0, 0}, 50};   // Left
  f.planes[1] = {{-1, 0, 0}, 50};  // Right
  f.planes[2] = {{0, 1, 0}, 50};   // Bottom
  f.planes[3] = {{0, -1, 0}, 50};  // Top
  f.planes[4] = {{0, 0, 1}, 50};   // Near
  f.planes[5] = {{0, 0, -1}, 50};  // Far
  return f;
}

}  // namespace

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
    planes[i].distance = d / -magnitude;
  }
}

// Test AABB vs. all 6 planes
bool Frustum::Intersects(const AABBf& aabb) const {
  for (int i = 0; i < 6; ++i) {
    if (aabb.IsOutsidePlane(planes[i]))
      return false;
  }
  return true;
}

bool Frustum::Intersects(const OBBf& obb, const Matrix4f& model) const {
  Matrix4f inverse_model;
  model.InverseOrthogonal(inverse_model);

  AABBf aabb;
  obb.GetLocalBox(aabb);

  // Transform each plane to the model's local space and test
  for (int i = 0; i < 6; i++) {
    Plane p = planes[i];
    p.Transform(inverse_model);

    if (aabb.IsOutsidePlane(p))
      return false;
  }
  return true;
}

std::unique_ptr<BVHNode> BuildBVHTree(std::vector<const MeshObject*>& objects) {
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
      obj->obb.GetLocalBox(aabb);
      aabb.min *= obj->model;
      aabb.max *= obj->model;
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
    std::sort(node_objects.begin(), node_objects.end(),
              [axis](const MeshObject* a, const MeshObject* b) {
                float ca, cb;
                if (axis == 0) {
                  ca = a->model.Row(3).x;
                  cb = b->model.Row(3).x;
                } else if (axis == 1) {
                  ca = a->model.Row(3).y;
                  cb = b->model.Row(3).y;
                } else {  // axis == 2
                  ca = a->model.Row(3).z;
                  cb = b->model.Row(3).z;
                }
                return ca < cb;
              });

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

/**
 * @brief Recursively prints a node and its children with ASCII art connectors.
 * @param node The current node to print.
 * @param prefix The string prefix that creates the connecting lines.
 * @param is_last True if this is the last child of its parent (a right child).
 */
void DumpBVHTree(const BVHNode* node, const std::string& prefix, bool is_last) {
  if (!node)
    return;

  std::ostringstream out;

  // Print the current node's line
  out << prefix;
  out << (is_last ? "└──" : "├──");

  // Print node details
  if (node->object) {
    out << "[Leaf] ID: " << node->object->id << " ";
  } else {
    out << "[Internal] ";
  }

  // Print bounding box info
  Vector3f center = (node->aabb.min + node->aabb.max) * 0.5f;
  float radius = (node->aabb.max - center).Length();
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

int TestBVH() {
  std::unique_ptr<BVHNode> root;

  // 1. Create a scene with some mesh objects
  std::vector<MeshObject> scene_objects = CreateTestObjects(1000);
  DLOG(0) << "Created " << scene_objects.size() << " objects in the scene.";

  // Build the BVH from the list of objects
  std::vector<const MeshObject*> object_pointers;
  object_pointers.reserve(scene_objects.size());
  for (const auto& obj : scene_objects)
    object_pointers.push_back(&obj);
  root = BuildBVHTree(object_pointers);
  DLOG(0) << "BVH built with initial objects.";

  //   bvh.dumpTree();

  // 2. Define a viewing frustum for culling
  Frustum view_frustum = CreateTestFrustum();

  // 3. Perform frustum culling
  std::vector<int> visible_objects = FrustumCull(root.get(), view_frustum);
  DLOG(0) << "Found " << visible_objects.size() << " visible objects.";
  DCHECK(visible_objects.size() == 163);

  // --- 4. Example of dynamic object management ---
  DLOG(0) << "\n--- Simulating dynamic updates ---";

  // Add a new object that should be visible
  MeshObject new_obj;
  new_obj.id = 1001;
  new_obj.obb.center = {0, 0, 0};
  new_obj.obb.extents = {5, 5, 5};
  new_obj.model.CreateTranslation({0, 0, 0});
  scene_objects.push_back(new_obj);
  DLOG(0) << "Added a new object (ID 1001) at the origin.";

  // Remove an existing object (ID 5)
  scene_objects.erase(
      std::remove_if(scene_objects.begin(), scene_objects.end(),
                     [](const MeshObject& obj) { return obj.id == 5; }),
      scene_objects.end());
  DLOG(0) << "Removed object with ID 5.";

  // Rebuild the BVH with the modified object list
  object_pointers.clear();
  object_pointers.reserve(scene_objects.size());
  for (const auto& obj : scene_objects)
    object_pointers.push_back(&obj);
  root = BuildBVHTree(object_pointers);
  DLOG(0) << "BVH rebuilt after updates.";

  // Perform culling again with the updated BVH
  visible_objects = FrustumCull(root.get(), view_frustum);
  DLOG(0) << "Found " << visible_objects.size()
          << " visible objects after update.";
  DCHECK(visible_objects.size() == 164);

  // Check if the new object is in the visible set
  bool found_new = false;
  for (int id : visible_objects) {
    if (id == 1001) {
      found_new = true;
      break;
    }
  }
  if (found_new) {
    DLOG(0) << "The newly added object (ID 1001) is visible as expected.";
  } else {
    NOTREACHED()
        << "Error: The newly added object was not found in the visible set.";
  }

  return 0;
}
