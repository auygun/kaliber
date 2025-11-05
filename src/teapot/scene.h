#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <vector>

#include "base/vecmath.h"
#include "engine/debug_layer.h"
#include "engine/model.h"
#include "engine/renderer/renderer_types.h"
#include "teapot/camera.h"
#include "teapot/ecs.h"

namespace eng {

class Renderer;

class Scene {
 public:
  Scene();
  ~Scene();

  void Create(Renderer* renderer);

  void Render(float frame_frac);

  void Update(float delta_time,
              const base::Vector2f& angles,
              const base::Vector3f& offset);

  void OnClick(const base::Vector2f& pos);

  void CreateProjectionMatrix();

  Camera& GetCamera() { return camera_; }

 private:
  struct WorldObject {
    Entity entity;
    size_t model_ind = (size_t)-1;
    base::OBBf obb;
    base::Matrix4f transform{1};
  };

  // The component for storing parent-child relationships and transformations of
  // world objects. This is the core of the scene graph.
  struct CoreDataComponent {
    std::string name;

    base::Matrix4f local_transform{1};
    base::Matrix4f world_transform{1};
    bool is_dirty{true};

    // Hierarchy (Doubly-Linked Sibling List)
    Entity parent{NULL_ENTITY};
    Entity first_child{NULL_ENTITY};
    Entity next_sibling{NULL_ENTITY};
    Entity prev_sibling{NULL_ENTITY};
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

  // Cached pointer to the core data pool for fast access.
  ComponentPool<CoreDataComponent>* core_data_pool_{nullptr};

  std::vector<BVHNode> bvh_tree_;

  Entity selected_entity_{NULL_ENTITY};

  bool show_bounding_volumes_ = false;

  SceneData scene_data_;
  LightData lights_[4];
  std::vector<InstanceData> instances_;

  uint64_t scene_data_ubo_ = 0;
  uint64_t lights_ubo_ = 0;
  uint64_t instances_ubo_ = 0;
  uint64_t scene_dset_ = 0;

  Renderer* renderer_ = nullptr;

  void UpdateViewProjectionMatrix();
  void UpdateFrustum();

  std::vector<std::tuple<size_t, size_t, size_t>> UpdateInstancesAndGetDrawList(
      const std::vector<Scene::WorldObject>& objects);

  void UploadSceneData();

  std::vector<BVHNode> BuildBVHTree(std::vector<WorldObject> objects);

  std::vector<WorldObject> FrustumCull(const std::vector<BVHNode>& nodes,
                                       const base::Frustumf& frustum);

  void DumpBVHTree(const std::vector<BVHNode>& nodes,
                   size_t node_ind,
                   const std::string& prefix,
                   bool is_last = true);

  void DrawBVHTree(const std::vector<BVHNode>& nodes, size_t node_ind);

  // Gets the entity's final world transform. Recalculates if dirty, recursively
  // updating the entity and all its ancestors.
  const base::Matrix4f& GetWorldTransform(Entity entity);

  // Marks this entity and all its descendants as dirty.
  void SetDirty(CoreDataComponent& core_data);

  void DetachFromParent(CoreDataComponent& core_data);

  // Detaches an entity from its current parent's child list and attaches to a
  // new parent.
  void SetParent(Entity entity, Entity new_parent);

  // Destroys an entity, its children, and updates its parent.
  void DestroyEntityAndChildren(Entity entity);

  // Creates a 3D world-space ray from 2D screen coordinates.
  base::Rayf CreateRayFromScreen(float screen_x, float screen_y);

  // Selects an entity by casting a ray.
  Entity SelectEntity(const std::vector<BVHNode>& nodes, const base::Rayf& ray);
};

}  // namespace eng

#endif  // TEAPOT_SCENE_H
