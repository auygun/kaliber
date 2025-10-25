#include "engine/scene_graph.h"

#include <deque>

#include "base/vecmath.h"

using namespace base;

namespace eng {

SceneNode::SceneNode(size_t model_ind, const std::string& name)
    : m_name{name}, model_ind{model_ind} {
  // Start with default, clean transforms
  m_localTransform.Unit();  // = Matrix4f::identity();
  m_worldTransform.Unit();  // = Matrix4f::identity();
}

// --- Hierarchy Management ---

/**
 * @brief Adds a child node. Takes ownership of the unique_ptr.
 * The child is automatically detached from its previous parent.
 */
void SceneNode::addChild(std::unique_ptr<SceneNode> child) {
  if (!child)
    return;

  // Detach from previous parent if one exists
  if (child->m_parent) {
    // Find the child in its parent's list and release it
    // This is a bit complex, so a helper is used.
    // A raw pointer comparison is safe here.
    child->m_parent->removeChild(child.get());
  }

  // Set new parent and add to list
  child->m_parent = this;
  m_children.push_back(std::move(child));
}

/**
 * @brief Removes a child node and returns ownership to the caller.
 * @param child Raw pointer to the child node to remove.
 * @return std::unique_ptr<SceneNode> to the detached child, or nullptr if not
 * found.
 */
std::unique_ptr<SceneNode> SceneNode::removeChild(SceneNode* child) {
  if (!child)
    return nullptr;

  auto found = std::find_if(m_children.begin(), m_children.end(),
                            [child](const std::unique_ptr<SceneNode>& p) {
                              return p.get() == child;
                            });

  if (found != m_children.end()) {
    // Found it. Move the unique_ptr out of the vector.
    std::unique_ptr<SceneNode> detachedChild = std::move(*found);
    m_children.erase(found);  // Erase the (now empty) unique_ptr
    detachedChild->m_parent = nullptr;
    return detachedChild;
  }
  return nullptr;
}

// --- Transform Management ---

void SceneNode::setPosition(const Vector3f& position) {
  m_position = position;
  setDirty();
}
void SceneNode::setRotation(const Quatf& rotation) {
  m_rotation = rotation;
  setDirty();
}
void SceneNode::setScale(const Vector3f& scale) {
  m_scale = scale;
  setDirty();
}

/**
 * @brief Gets the node's final world transform.
 * Recalculates if dirty.
 */
const Matrix4f& SceneNode::getWorldTransform() {
  if (m_isDirty) {
    // 1. Recalculate local transform
    m_localTransform.Create(m_rotation, m_position);

    // 2. Combine with parent's world transform
    if (m_parent) {
      m_parent->getWorldTransform().Multiply(m_localTransform,
                                             m_worldTransform);
    } else {
      m_worldTransform = m_localTransform;  // This is the root node
    }

    // 3. Mark as clean
    m_isDirty = false;
  }
  return m_worldTransform;
}

// --- Main Game Loop Functions ---

std::vector<WorldObject> SceneNode::GetWorldObjects(
    const std::vector<eng::Model>& models) {
  std::vector<WorldObject> world_objects;
  std::deque<SceneNode*> stack;
  stack.push_back(this);

  size_t index = 0;
  while (!stack.empty()) {
    auto* node = stack.back();
    stack.pop_back();

    if (node->model_ind != (size_t)-1) {
      OBBf obb{node->getWorldTransform(), models[node->model_ind].GetExtents()};
      world_objects.emplace_back(index++, node->model_ind, obb,
                                 node->getWorldTransform());
    }

    for (auto& child : node->m_children)
      stack.push_back(child.get());
  }

  return world_objects;
}

void SceneNode::setDirty() {
  if (m_isDirty)
    return;  // Already dirty, no need to propagate

  m_isDirty = true;
  for (auto& child : m_children) {
    child->setDirty();
  }
}

}  // namespace eng
