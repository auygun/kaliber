#include "teapot/bvh.h"

#include "base/log.h"

using namespace base;

// --- Frustum Intersection Implementations ---

// Test sphere vs. all 6 planes
bool Frustum::intersects(const BoundingSphere& sphere) const {
  for (int i = 0; i < 6; ++i) {
    // Calculate signed distance from sphere center to plane
    float dist = planes[i].normal.x * sphere.center.x +
                 planes[i].normal.y * sphere.center.y +
                 planes[i].normal.z * sphere.center.z + planes[i].distance;
    // If center is outside the plane by more than the radius, it's culled
    if (dist < -sphere.radius) {
      return false;
    }
  }
  return true;
}

// Test AABB vs. all 6 planes
bool Frustum::intersects(const AABB& aabb) const {
  for (int i = 0; i < 6; ++i) {
    // Find the p-vertex (positive) and n-vertex (negative)
    Vec3 p_vertex = aabb.min;

    if (planes[i].normal.x >= 0) {
      p_vertex.x = aabb.max.x;
    }
    if (planes[i].normal.y >= 0) {
      p_vertex.y = aabb.max.y;
    }
    if (planes[i].normal.z >= 0) {
      p_vertex.z = aabb.max.z;
    }

    // If p-vertex is outside, the whole box is outside
    float dist = planes[i].normal.x * p_vertex.x +
                 planes[i].normal.y * p_vertex.y +
                 planes[i].normal.z * p_vertex.z + planes[i].distance;
    if (dist < 0) {
      return false;
    }
  }
  return true;
}

// Test OBB vs. all 6 planes
bool Frustum::intersects(const OBB& obb) const {
  // Note: A truly robust OBB-Frustum test uses the Separating Axis Theorem
  // (SAT). This simplified version checks the 8 corners of the OBB against the
  // frustum planes. It's mostly correct but can fail in some edge cases (e.g.,
  // a long frustum edge poking through an OBB face).
  Vec3 corners[8];
  Vec3 v_x = {obb.axes[0].x * obb.extents.x, obb.axes[0].y * obb.extents.x,
              obb.axes[0].z * obb.extents.x};
  Vec3 v_y = {obb.axes[1].x * obb.extents.y, obb.axes[1].y * obb.extents.y,
              obb.axes[1].z * obb.extents.y};
  Vec3 v_z = {obb.axes[2].x * obb.extents.z, obb.axes[2].y * obb.extents.z,
              obb.axes[2].z * obb.extents.z};

  corners[0] = {obb.center.x - v_x.x - v_y.x - v_z.x,
                obb.center.y - v_x.y - v_y.y - v_z.y,
                obb.center.z - v_x.z - v_y.z - v_z.z};
  // ... calculate other 7 corners similarly

  // For this simplified example, we'll just check the center with an expanded
  // radius as a proxy
  float max_extent = std::max({obb.extents.x, obb.extents.y, obb.extents.z});
  BoundingSphere proxySphere = {obb.center,
                                max_extent * 1.74f};  // 1.74 is ~sqrt(3)
  return this->intersects(proxySphere);
}

Frustum Frustum::createFromMatrix(const Matrix4f& vp) {
  Frustum frustum;

  // Left Plane: row4 + row1
  Vector4f n[6];
  n[0] = vp.Row4(3) + vp.Row4(0);
  n[1] = vp.Row4(3) - vp.Row4(0);
  n[2] = vp.Row4(3) + vp.Row4(1);
  n[3] = vp.Row4(3) - vp.Row4(1);
  n[4] = vp.Row4(3) + vp.Row4(2);
  n[5] = vp.Row4(3) - vp.Row4(2);

  for (int i = 0; i < 6; ++i) {
    n[i].Normalize();

    frustum.planes[i].normal.x = n[i].x;
    frustum.planes[i].normal.y = n[i].y;
    frustum.planes[i].normal.z = n[i].z;
    frustum.planes[i].distance = n[i].w;
  }
  return frustum;
}

// --- BVH Method Implementations ---

void BVH::build(const std::vector<MeshObject>& objects) {
  if (objects.empty()) {
    root = nullptr;
    return;
  }
  // Create a vector of pointers to the objects to be sorted recursively
  std::vector<const MeshObject*> objectPointers;
  objectPointers.reserve(objects.size());
  for (const auto& obj : objects) {
    objectPointers.push_back(&obj);
  }
  root = buildRecursive(objectPointers);
}

std::unique_ptr<BVHNode> BVH::buildRecursive(
    std::vector<const MeshObject*>& objects) {
  auto node = std::make_unique<BVHNode>();

  // --- 1. Leaf Node Case ---
  // If only one object remains, create a leaf node.
  if (objects.size() == 1) {
    node->object = objects[0];

    // Create tight-fitting bounds for the single object
    node->aabb = node->object->initialAABB;

    // Sphere from AABB
    node->sphere.center = {(node->aabb.min.x + node->aabb.max.x) * 0.5f,
                           (node->aabb.min.y + node->aabb.max.y) * 0.5f,
                           (node->aabb.min.z + node->aabb.max.z) * 0.5f};
    Vec3 extent = {node->aabb.max.x - node->sphere.center.x,
                   node->aabb.max.y - node->sphere.center.y,
                   node->aabb.max.z - node->sphere.center.z};
    node->sphere.radius = std::sqrt(extent.x * extent.x + extent.y * extent.y +
                                    extent.z * extent.z);

    // OBB from AABB (axis-aligned in this simple case)
    node->obb.center = node->sphere.center;
    node->obb.extents = {node->aabb.max.x - node->obb.center.x,
                         node->aabb.max.y - node->obb.center.y,
                         node->aabb.max.z - node->obb.center.z};
    return node;
  }

  // --- 2. Internal Node Case ---
  // Calculate the combined AABB for all objects in this node
  for (const auto& obj : objects) {
    node->aabb.expand(obj->initialAABB);
  }

  // Find the longest axis of the combined AABB to split along
  Vec3 extent = {node->aabb.max.x - node->aabb.min.x,
                 node->aabb.max.y - node->aabb.min.y,
                 node->aabb.max.z - node->aabb.min.z};
  int axis = 0;
  if (extent.y > extent.x)
    axis = 1;
  if (extent.z > extent.y)
    axis = 2;

  // Sort objects along the chosen axis based on their center point
  std::sort(objects.begin(), objects.end(),
            [axis](const MeshObject* a, const MeshObject* b) {
              float centerA = 0, centerB = 0;
              switch (axis) {
                case 0:
                  centerA = a->initialAABB.min.x + a->initialAABB.max.x;
                  centerB = b->initialAABB.min.x + b->initialAABB.max.x;
                  break;
                case 1:
                  centerA = a->initialAABB.min.y + a->initialAABB.max.y;
                  centerB = b->initialAABB.min.y + b->initialAABB.max.y;
                  break;
                case 2:
                  centerA = a->initialAABB.min.z + a->initialAABB.max.z;
                  centerB = b->initialAABB.min.z + b->initialAABB.max.z;
                  break;
              }
              return centerA < centerB;
            });

  // Split the objects into two halves
  size_t mid = objects.size() / 2;
  std::vector<const MeshObject*> leftObjects(objects.begin(),
                                             objects.begin() + mid);
  std::vector<const MeshObject*> rightObjects(objects.begin() + mid,
                                              objects.end());

  // Recursively build child nodes
  node->left = buildRecursive(leftObjects);
  node->right = buildRecursive(rightObjects);

  // This internal node's AABB is already computed. Now compute its bounding
  // sphere. A simple sphere that contains the children's spheres.
  Vec3 center_dist = {
      node->right->sphere.center.x - node->left->sphere.center.x,
      node->right->sphere.center.y - node->left->sphere.center.y,
      node->right->sphere.center.z - node->left->sphere.center.z};
  float dist =
      std::sqrt(center_dist.x * center_dist.x + center_dist.y * center_dist.y +
                center_dist.z * center_dist.z);
  node->sphere.radius =
      (dist + node->left->sphere.radius + node->right->sphere.radius) * 0.5f;
  Vec3 dir = {center_dist.x / dist, center_dist.y / dist, center_dist.z / dist};
  node->sphere.center = {
      node->left->sphere.center.x +
          dir.x * (node->sphere.radius - node->left->sphere.radius),
      node->left->sphere.center.y +
          dir.y * (node->sphere.radius - node->left->sphere.radius),
      node->left->sphere.center.z +
          dir.z * (node->sphere.radius - node->left->sphere.radius)};

  return node;
}

std::vector<int> BVH::frustumCull(const Frustum& frustum) const {
  std::vector<int> visibleObjectIDs;
  if (!root) {
    return visibleObjectIDs;
  }
  frustumCullRecursive(root.get(), frustum, visibleObjectIDs);
  return visibleObjectIDs;
}

void BVH::frustumCullRecursive(const BVHNode* node,
                               const Frustum& frustum,
                               std::vector<int>& visibleObjectIDs) const {
  // Hierarchical Culling:
  // 1. Coarse Sphere test
  if (!frustum.intersects(node->sphere)) {
    return;
  }

  // Finer AABB/OBB test
  if (!frustum.intersects(node->aabb)) {
    return;
  }

  if (node->isLeaf()) {
    if (frustum.intersects(node->obb)) {
      visibleObjectIDs.push_back(node->object->id);
    }
    return;
  }

  // If it's an internal node that passed tests, check its children
  if (node->left) {
    frustumCullRecursive(node->left.get(), frustum, visibleObjectIDs);
  }
  if (node->right) {
    frustumCullRecursive(node->right.get(), frustum, visibleObjectIDs);
  }
}

/**
 * @brief Dumps a visual representation of the entire BVH tree to the console.
 */
void BVH::dumpTree() const {
  DLOG(0) << "\n--- BVH Tree Dump ---";
  if (root) {
    dumpNodeRecursive(root.get(), "", true);
  } else {
    DLOG(0) << "[Empty Tree]";
  }
  DLOG(0) << "---------------------";
}

/**
 * @brief Recursively prints a node and its children with ASCII art connectors.
 * @param node The current node to print.
 * @param prefix The string prefix that creates the connecting lines.
 * @param isLast True if this is the last child of its parent (a right child).
 */
void BVH::dumpNodeRecursive(const BVHNode* node,
                            const std::string& prefix,
                            bool isLast) const {
  if (!node)
    return;

  std::ostringstream out;

  // Print the current node's line
  out << prefix;
  out << (isLast ? "└──" : "├──");

  // Print node details
  if (node->object) {
    out << "[Leaf] ID: " << node->object->id << " ";
  } else {
    out << "[Internal] ";
  }

  // Print bounding box info
  out << "Center: (" << node->sphere.center.x << ", " << node->sphere.center.y
      << ", " << node->sphere.center.z << ")";
  DLOG(0) << out.str();

  // Prepare the prefix for the children
  std::string childPrefix = prefix + (isLast ? "    " : "│   ");

  // Recurse for children (if they exist)
  if (!node->object) {
    // The right child is always the "last" one for its parent
    dumpNodeRecursive(node->left.get(), childPrefix, false);
    dumpNodeRecursive(node->right.get(), childPrefix, true);
  }
}

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

    obj.initialAABB.min = {x - size, y - size, z - size};
    obj.initialAABB.max = {x + size, y + size, z + size};
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

int TestBVH() {
  BVH bvh;

  // 1. Create a scene with some mesh objects
  std::vector<MeshObject> sceneObjects = CreateTestObjects(1000);
  DLOG(0) << "Created " << sceneObjects.size() << " objects in the scene.";

  // Build the BVH from the list of objects
  bvh.build(sceneObjects);
  DLOG(0) << "BVH built with initial objects.";

//   bvh.dumpTree();

  // 2. Define a viewing frustum for culling
  Frustum viewFrustum = CreateTestFrustum();

  // 3. Perform frustum culling
  std::vector<int> visibleObjects = bvh.frustumCull(viewFrustum);
  DLOG(0) << "Found " << visibleObjects.size() << " visible objects.";
  DCHECK(visibleObjects.size() == 163);

  // --- 4. Example of dynamic object management ---
  DLOG(0) << "\n--- Simulating dynamic updates ---";

  // Add a new object that should be visible
  MeshObject newObj;
  newObj.id = 1001;
  newObj.initialAABB.min = {0, 0, 0};
  newObj.initialAABB.max = {5, 5, 5};
  sceneObjects.push_back(newObj);
  DLOG(0) << "Added a new object (ID 1001) at the origin.";

  // Remove an existing object (ID 5)
  sceneObjects.erase(
      std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                     [](const MeshObject& obj) { return obj.id == 5; }),
      sceneObjects.end());
  DLOG(0) << "Removed object with ID 5.";

  // Rebuild the BVH with the modified object list
  bvh.build(sceneObjects);
  DLOG(0) << "BVH rebuilt after updates.";

  // Perform culling again with the updated BVH
  visibleObjects = bvh.frustumCull(viewFrustum);
  DLOG(0) << "Found " << visibleObjects.size()
          << " visible objects after update.";
  DCHECK(visibleObjects.size() == 164);

  // Check if the new object is in the visible set
  bool foundNew = false;
  for (int id : visibleObjects) {
    if (id == 1001) {
      foundNew = true;
      break;
    }
  }
  if (foundNew) {
    DLOG(0) << "The newly added object (ID 1001) is visible as expected.";
  } else {
    NOTREACHED()
        << "Error: The newly added object was not found in the visible set.";
  }

  return 0;
}
