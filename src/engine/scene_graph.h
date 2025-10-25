#ifndef ENGINE_SCENE_GRAPH_H
#define ENGINE_SCENE_GRAPH_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "base/vecmath.h"
#include "engine/model.h"
#include "engine/bvh.h"

using namespace base;

namespace eng {

// struct WorldObject {
//   size_t id;
//   size_t model_ind = (size_t)-1;
//   base::OBBf obb;
//   base::Matrix4f transform;
// };

// --- 3. THE SCENE NODE ---

/**
 * @brief The core building block of the scene graph.
 * Manages its own transform, components, and children.
 * Ownership of children and components is handled via std::unique_ptr.
 */
class SceneNode {
 public:
  SceneNode(size_t model_ind, const std::string& name = "SceneNode");

  // --- Hierarchy Management ---

  /**
   * @brief Adds a child node. Takes ownership of the unique_ptr.
   * The child is automatically detached from its previous parent.
   */
  void addChild(std::unique_ptr<SceneNode> child);

  /**
   * @brief Removes a child node and returns ownership to the caller.
   * @param child Raw pointer to the child node to remove.
   * @return std::unique_ptr<SceneNode> to the detached child, or nullptr if not
   * found.
   */
  std::unique_ptr<SceneNode> removeChild(SceneNode* child);

  // --- Transform Management ---

  void setPosition(const Vector3f& position);
  void setRotation(const Quatf& rotation);
  void setScale(const Vector3f& scale);

  /**
   * @brief Gets the node's final world transform.
   * Recalculates if dirty.
   */
  const Matrix4f& getWorldTransform();

  // --- Getters ---
  size_t GetModelIndex() const { return model_ind; }
  const std::string& getName() const { return m_name; }
  SceneNode* getParent() const { return m_parent; }
  // --- Main Game Loop Functions ---

  std::vector<WorldObject> GetWorldObjects(
      const std::vector<eng::Model>& models);

 private:
  /**
   * @brief Marks this node and all its descendants as dirty.
   * This forces a transform recalculation on the next update().
   */
  void setDirty();

  // --- Transform Data ---
  Vector3f m_position{0};
  Quatf m_rotation{0, 0, 0, 1};
  Vector3f m_scale{1.0f};

  // --- Cached Matrices ---
  Matrix4f m_localTransform{1};
  Matrix4f m_worldTransform{1};
  bool m_isDirty = true;  // Start dirty to force initial calculation

  // --- Hierarchy ---
  std::string m_name;
  SceneNode* m_parent = nullptr;
  std::vector<std::unique_ptr<SceneNode>> m_children;

  size_t model_ind = (size_t)-1;
};

}  // namespace eng

#endif  // ENGINE_SCENE_GRAPH_H
