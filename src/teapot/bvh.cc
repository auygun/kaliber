#include "teapot/bvh.h"

#include "base/log.h"

using namespace base;

// --- Frustum Intersection Implementations ---

// Test sphere vs. all 6 planes
bool Frustum::Intersects(const BoundingSphere& sphere) const {
  for (int i = 0; i < 6; ++i) {
    // Calculate signed distance from sphere center to plane
    float dist =
        planes[i].normal.DotProduct(sphere.center) + planes[i].distance;
    if (dist < -sphere.radius) {
      return false;
    }
  }
  return true;
}

// Test AABB vs. all 6 planes
bool Frustum::Intersects(const AABB& aabb) const {
  for (int i = 0; i < 6; ++i) {
    // Find the positive vertex (corner of AABB extending furthest in the
    // direction of the plane's normal)
    Vector3f p_vertex = aabb.min;
    if (planes[i].normal.x >= 0)
      p_vertex.x = aabb.max.x;
    if (planes[i].normal.y >= 0)
      p_vertex.y = aabb.max.y;
    if (planes[i].normal.z >= 0)
      p_vertex.z = aabb.max.z;

    // If p_vertex is outside, the whole box is outside
    float dist = planes[i].normal.DotProduct(p_vertex) + planes[i].distance;
    if (dist < 0)
      return false;
  }
  return true;
}

// static
AABB AABB::CreateFromOBB(const OBB& obb) {
  AABB aabb;
  for (int i = 0; i < 8; ++i) {
    Vector3f p = obb.center;
    p = p + obb.axes[0] * obb.extents.x * ((i & 1) ? 1.0f : -1.0f);
    p = p + obb.axes[1] * obb.extents.y * ((i & 2) ? 1.0f : -1.0f);
    p = p + obb.axes[2] * obb.extents.z * ((i & 4) ? 1.0f : -1.0f);
    aabb.expand(p);
  }
  return aabb;
}

void Plane::Translate(const base::Vector3f& v) {
  distance -= normal.DotProduct(v);
}

void Plane::Transform(const base::Matrix4f& mat) {
  normal.MultiplyMatrix3x3(mat);
  Translate(mat.Row(3));
}

bool Frustum::Intersects(const OBB& obb, const Matrix4f& model) const {
  Matrix4f inverse_model;
  model.InverseOrthogonal(inverse_model);

  // Create a new frustum in the model's local space.
  Frustum local_frustum;
  for (int i = 0; i < 6; i++) {
    local_frustum.planes[i] = planes[i];
    local_frustum.planes[i].Transform(inverse_model);
  }

  // obb is in the local space
  AABB aabb = AABB::CreateFromOBB(obb);
  return local_frustum.Intersects(aabb);
}

Frustum Frustum::CreateFromMatrix(const Matrix4f& vp) {
  Frustum frustum;

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
    frustum.planes[i].normal = n / magnitude;
    frustum.planes[i].distance = d / -magnitude;
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
    node->aabb = AABB::CreateFromOBB(node->object->obb);
    node->aabb.min *= node->object->model;
    node->aabb.max *= node->object->model;
  } else {
    // --- 2. Internal Node Case ---
    // Calculate the combined AABB for all objects in this node
    for (const auto& obj : objects) {
      AABB aabb = AABB::CreateFromOBB(obj->obb);
      aabb.min *= obj->model;
      aabb.max *= obj->model;
      node->aabb.expand(aabb);
    }

    // Find the longest axis of the combined AABB to split along
    Vector3f extent = node->aabb.max - node->aabb.min;
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;
    if (extent.z > extent.y)
      axis = 2;

    // Sort objects along the chosen axis based on their center point
    std::sort(objects.begin(), objects.end(),
              [axis](const MeshObject* a, const MeshObject* b) {
                float ca, cb;
                if (axis == 0) {
                  ca = a->model.Row(3).x;
                  cb = b->model.Row(3).x;
                } else if (axis == 1) {
                  ca = a->model.Row(3).y;
                  cb = b->model.Row(3).y;
                } else if (axis == 2) {
                  ca = a->model.Row(3).z;
                  cb = b->model.Row(3).z;
                }
                return ca < cb;
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
  }

  // Sphere from AABB
  node->sphere.center = (node->aabb.min + node->aabb.max) * 0.5f;
  Vector3f extent = node->aabb.max - node->sphere.center;
  node->sphere.radius = extent.Length();

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
  if (!frustum.Intersects(node->sphere))
    return;

  if (node->isLeaf()) {
    if (frustum.Intersects(node->object->obb, node->object->model))
      visibleObjectIDs.push_back(node->object->id);
    return;
  } else if (!frustum.Intersects(node->aabb)) {
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
  newObj.obb.center = {0, 0, 0};
  newObj.obb.extents = {5, 5, 5};
  newObj.model.CreateTranslation({0, 0, 0});
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
