#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <vector>

#include "base/vecmath.h"
#include "engine/debug_layer.h"
#include "engine/drawable.h"
#include "engine/model.h"
#include "engine/renderer/renderer_types.h"
#include "teapot/camera.h"
#include "teapot/ecs.h"

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

 private:
  struct WorldObject {
    size_t model_ind = (size_t)-1;
    base::OBBf obb;
    base::Matrix4f transform{1};
  };

  struct SceneNodeComponent {
    std::string name;

    base::Matrix4f local_transform{1};
    base::Matrix4f world_transform{1};
    bool is_dirty{true};

    Entity parent{NULL_ENTITY};
    std::vector<Entity> children{};

    void AddChild(Entity child_entity) { children.push_back(child_entity); }
  };

  struct ModelComponent {
    size_t model_index{(size_t)-1};
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

  // ECS Registry for all entities in the world.
  Registry registry_;
  Entity root_entity_{NULL_ENTITY};

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
   * @brief Gets the node's final world transform. Recalculates if dirty.
   * This is a recursive call; it will update the node and all its ancestors.
   * @param index The index of the node.
   * @return A const reference to the node's cached world_transform.
   */
  const base::Matrix4f& GetWorldTransform(SceneNodeComponent& node);

  /**
   * @brief Marks this node and all its descendants as dirty.
   * @param index The index of the node to start from.
   */
  void SetDirty(SceneNodeComponent& node);

  /**
   * @brief Recursively destroys an entity, its children, and updates its
   * parent.
   *
   * This is the "smart" way to destroy an entity in a scene graph.
   */
  void DestroyEntityAndChildren(Entity entity);
};

#endif  // TEAPOT_SCENE_H
