#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <string>
#include <vector>

#include "base/vecmath.h"
#include "engine/debug_layer.h"
#include "engine/model.h"
#include "engine/renderer/renderer_types.h"
#include "teapot/fly_camera.h"
#include "teapot/components.h"
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

  // Camera& GetCamera() { return camera_; }

  Registry& GetRegistry() { return registry_; }

 private:
  // The temporary struct used for building the BVH tree.
  // Keep it lightweight. Bloated struct will make the sort slower.
  struct BVHBuildItem {
    Entity entity;
    base::AABBf aabb;
    base::Vector3f center;
  };

  struct BVHNode {
    // Bounding volume for the node
    base::AABBf aabb;

    // Tree structure
    uint32_t left{NULL_INDEX};
    uint32_t right{NULL_INDEX};

    // Payload
    Entity entity;

    bool IsLeaf() const { return left == NULL_INDEX && right == NULL_INDEX; }
  };

  // Lightweight temporary struct that contains the data needed for sorting
  // visible entities by model index.
  struct SortItem {
    Entity entity;
    uint32_t model_index;

    bool operator<(const SortItem& other) const {
      return model_index < other.model_index;
    }
  };

  // The data needed for the final draw call.
  struct DrawData {
    uint32_t model_index;
    uint32_t first_instance;
    uint32_t instance_count;
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


  // Camera camera_;
  FlyCamera fly_camera_; // TODO: Remove.
  base::Frustumf frustum_;

  eng::DebugLayer debug_layer_;

  // ECS Registry for all entities in the world.
  Registry registry_;

  // Root of the scene-graph.
  Entity root_entity_{NULL_ENTITY};

  // Cached pointers to globals.
  RenderContext* render_context_{nullptr};

  // Cached pointers to the pools for fast access.
  ComponentPool<SceneNodeComponent>* scene_node_pool_{nullptr};
  ComponentPool<WorldTransformComponent>* world_transform_pool_{nullptr};
  ComponentPool<LocalTransformComponent>* local_transform_pool_{nullptr};
  ComponentPool<WorldTransformDirtyTag>* dirty_tag_pool_{nullptr};
  ComponentPool<WorldBoundsComponent>* world_bounds_pool_{nullptr};
  ComponentPool<ModelComponent>* model_pool_{nullptr};

  // A list that keeps list of entity IDs for every possible depth level.
  std::vector<std::vector<Entity>> depth_buckets_;

  std::vector<BVHNode> bvh_tree_;

  Entity selected_entity_{NULL_ENTITY};

  bool show_bounding_volumes_ = false;

  SceneData scene_data_;
  LightData lights_[4];
  std::vector<InstanceData> instances_;  // TODO: remove

  uint64_t scene_data_ubo_ = 0;
  uint64_t lights_ubo_ = 0;
  uint64_t instances_ubo_ = 0;
  uint64_t scene_dset_ = 0;

  Renderer* renderer_ = nullptr;

  Entity NewEntity(Entity parent,
                   uint32_t model_index,
                   const base::Matrix4f& transform);

  void UpdateRenderContext();

  // Processes a sorted list of visible entities and groups them into instanced
  // draw calls (batches).
  // Populates the member 'instances_' vector with the world transforms.
  // Returns a draw list where each DrawData item represents a single instanced
  // draw call (a batch).
  std::vector<DrawData> BuildDrawList(std::vector<SortItem> visible_entities);

  void UploadSceneData();

  void BuildBVHTree(std::vector<BVHBuildItem> items);

  // Traverses the BVH and builds a list of visible entities decorated with the
  // data needed for sorting (the model_index).
  std::vector<SortItem> FrustumCull(const base::Frustumf& frustum);

  void DumpBVHTree(const std::vector<BVHNode>& nodes,
                   uint32_t node_index,
                   const std::string& prefix,
                   bool is_last = true);

  void DrawBVHTree(const std::vector<BVHNode>& nodes, uint32_t node_index);

  void DetachFromParent(SceneNodeComponent& core_data);

  // Detaches an entity from its current parent's child list and attaches to a
  // new parent.
  void SetParent(Entity entity, Entity new_parent);

  // Destroys an entity, its children, and updates its parent.
  void DestroyEntityAndChildren(Entity entity);

  // Called whenever an entity is created, destroyed, or re-parented.
  void OnHierarchyChanged(Entity entity, uint32_t new_depth);

  // Updates WorldTransformComponent for objects that were tagged as dirty.
  void UpdateWoldTransforms();

  // Updates WorldBoundsComponent based on finalized transforms.
  void UpdateWorldBounds();

  // Creates a 3D world-space ray from 2D screen coordinates.
  base::Rayf CreateRayFromScreen(float screen_x, float screen_y);

  // Selects an entity by casting a ray.
  Entity SelectEntity(const std::vector<BVHNode>& nodes, const base::Rayf& ray);
};

}  // namespace eng

#endif  // TEAPOT_SCENE_H
