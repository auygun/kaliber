#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <vector>

#include "base/vecmath.h"
#include "engine/debug_layer.h"
#include "engine/drawable.h"
#include "engine/model.h"
#include "engine/renderer/renderer_types.h"
#include "teapot/camera.h"

class Scene : public eng::Drawable {
 public:
  /**
   * @brief A sentinel value representing "no node", equivalent to nullptr.
   */
  static const size_t NO_NODE = (size_t)-1;

  Scene();
  ~Scene();

  void Create();

  void Draw(float frame_frac) override;

  void Update(float delta_time, const base::Vector2f& angles, float zoom);

  void CreateProjectionMatrix();

  Camera& GetCamera() { return camera_; }

  /**
   * @brief Creates a new node as a child of the given parent.
   * @param parent_index The index of the parent node.
   * @param model_ind The "payload" model index for this node.
   * @param name An optional name for the node.
   * @return The index of the newly created node.
   */
  size_t CreateNode(size_t parent_index,
                    size_t model_ind,
                    const std::string& name = "SceneNode");

  /**
   * @brief Deletes a node and recursively deletes all of its children.
   * The node is returned to the free list for reuse.
   * @param index The index of the node to delete.
   */
  void DeleteNode(size_t index);

  /**
   * @brief Attaches a node to a new parent.
   * The node is automatically detached from its previous parent, if any.
   * @param parent_index The index of the new parent.
   * @param child_index The index of the child to move.
   */
  void SetParent(size_t parent_index, size_t child_index);

  // --- Getters ---
  Node& GetNode(size_t index) { return nodes_[index]; }
  const Node& GetNode(size_t index) const { return nodes_[index]; }
  size_t GetRootIndex() const { return root_index_; }
  size_t GetNodeCount() const { return nodes_.size(); }

  // --- Transformations ---
  void SetPosition(size_t index, const base::Vector3f& p);
  void SetRotation(size_t index, const base::Quatf& r);
  void SetScale(size_t index, float s);

  /**
   * @brief Gets the node's final world transform. Recalculates if dirty.
   * This is a recursive call; it will update the node and all its ancestors.
   * @param index The index of the node.
   * @return A const reference to the node's cached world_transform.
   */
  const base::Matrix4f& GetWorldTransform(size_t index);

 private:
  struct WorldObject {
    size_t model_ind = (size_t)-1;
    base::OBBf obb;
    base::Matrix4f transform{1};
  };

  /**
   * @brief Represents a single node in the scene.
   * This is a simple data struct; all logic is in the Scene class.
   */
  struct Node {
    // --- Transform Data ---
    base::Vector3f position{0};
    base::Quatf rotation{0, 0, 0, 1};
    float scale{1.0f};

    // --- Cached Matrices ---
    base::Matrix4f local_transform{1};
    base::Matrix4f world_transform{1};
    bool is_dirty = true;

    // --- Hierarchy (Indices) ---
    // We use a doubly-linked sibling list for O(1) child removal.
    size_t parent = NO_NODE;
    size_t first_child = NO_NODE;
    size_t next_sibling = NO_NODE;
    size_t prev_sibling = NO_NODE;  // For O(1) removal

    // --- Payload & State ---
    std::string name;
    size_t model_ind = NO_NODE;
    bool is_active = false;  // True if this slot in the vector is in use
  };

  struct BVHNode {
    // Bounding volume for the node
    base::AABBf aabb;

    // Tree structure
    size_t left = (size_t)-1;
    size_t right = (size_t)-1;

    // Payload
    WorldObject object;

    bool IsLeaf() const { return object.model_ind != (size_t)-1; }
  };

  // --- UBO ---

  struct SceneData {
    base::Matrix4f view_projection;
    base::Vector3f cam_pos;
    float white = 5.0f;
    base::Vector3f light_dir;
    float exposure = 1.0f;
    base::Vector3f light_radiance;
    float _pad0;
  };

  struct LightData {
    base::Vector3f pos;
    float power = 0;
  };

  struct InstanceData {
    base::Matrix4f model;
  };

  eng::VertexDescription vertex_description_;
  uint64_t shader_id_;
  std::vector<eng::Model> models_;

  Camera camera_;
  base::Matrix4f projection_;
  base::Frustumf frustum_;

  eng::DebugLayer debug_layer_;

  // The single, contiguous block of memory for all nodes.
  std::vector<Node> nodes_;
  // The index of the scene's root node (always 0).
  size_t root_index_ = 0;

  // The head of a intrusive linked list of unused node indices.
  // We use the `next_sibling` field of inactive nodes to store this list.
  size_t free_list_head_ = NO_NODE;

  std::vector<BVHNode> bvh_tree_;

  base::Vector3f albedo_{0.8f, 0.4f, 0.2f};
  float metallic_ = 1.0f;
  float roughness_ = 0.3f;
  float ao_ = 0.5f;

  SceneData scene_data_;
  LightData lights_[4];
  std::vector<InstanceData> instances_;

  uint64_t scene_data_ubo_ = 0;
  uint64_t lights_ubo_ = 0;
  uint64_t instances_ubo_ = 0;
  uint64_t scene_dset_ = 0;

  void UpdateViewProjectionMatrix();
  void UpdateFrustum();

  std::vector<std::tuple<size_t, size_t, size_t>> UpdateInstancesAndGetDrawList(
      const std::vector<Scene::WorldObject>& objects);

  void UploadSceneData();

  std::vector<WorldObject> GetSceneObjects();

  std::vector<BVHNode> BuildBVHTree(std::vector<WorldObject> objects);

  std::vector<WorldObject> FrustumCull(const std::vector<BVHNode>& nodes,
                                       const base::Frustumf& frustum);

  void DumpBVHTree(const std::vector<BVHNode>& nodes,
                   size_t node_ind,
                   const std::string& prefix,
                   bool is_last = true);

  void DrawBVHTree(const std::vector<BVHNode>& nodes, size_t node_ind);

  /**
   * @brief Detaches a node from its current parent's sibling list.
   * Helper function for SetParent and DeleteNode.
   * Assumes node_index has a valid parent.
   * @param node_index The index of the node to detach.
   */
  void DetachFromParent(size_t node_index);

  /**
   * @brief Attaches a node to a parent's sibling list.
   * @param parent_index Index of the new parent.
   * @param child_index Index of the child to attach.
   */
  void AttachToParent(size_t parent_index, size_t child_index);

  /**
   * @brief Marks this node and all its descendants as dirty.
   * @param index The index of the node to start from.
   */
  void SetDirty(size_t index);

  /**
   * @brief Gets an unused node index from the free list, or creates a new one.
   * @return A valid index.
   */
  size_t AllocateNode();

  /**
   * @brief Returns a node index to the free list for reuse.
   * @param index The index to free.
   */
  void FreeNode(size_t index);
};

#endif  // TEAPOT_SCENE_H
