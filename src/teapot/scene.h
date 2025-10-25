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

  class SceneNode {
   public:
    SceneNode(size_t model_ind, const std::string& name = "SceneNode");

    // Adds a child node. Takes ownership of the unique_ptr. The child is
    // automatically detached from its previous parent.
    void AddChild(std::unique_ptr<SceneNode> child);

    // Removes a child node and returns ownership to the caller.
    std::unique_ptr<SceneNode> RemoveChild(SceneNode* child);

    // Transformations
    void SetPosition(const base::Vector3f& p);
    void SetRotation(const base::Quatf& r);
    void SetScale(float s);

    // Gets the node's final world transform. Recalculates if dirty.
    const base::Matrix4f& GetWorldTransform();

    size_t GetModelIndex() const { return model_ind; }
    const std::string& GetName() const { return name; }
    SceneNode* GetParent() const { return parent; }

    std::vector<WorldObject> GetWorldObjects(
        const std::vector<eng::Model>& models);

   private:
    // Marks this node and all its descendants as dirty. This forces a transform
    // recalculation on the next update().
    void SetDirty();

    // Transform data
    base::Vector3f position{0};
    base::Quatf rotation{0, 0, 0, 1};
    float scale{1.0f};  // Uniform scale

    // Cached matrices
    base::Matrix4f local_transform{1};
    base::Matrix4f world_transform{1};
    bool is_dirty = true;  // Start dirty to force initial calculation

    // Hierarchy
    std::string name;
    SceneNode* parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children;

    // Payload
    size_t model_ind = (size_t)-1;
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

  SceneNode scene_root_{(size_t)-1, "Root"};
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

  std::vector<BVHNode> BuildBVHTree(std::vector<WorldObject> objects);

  std::vector<WorldObject> FrustumCull(const std::vector<BVHNode>& nodes,
                                       const base::Frustumf& frustum);

  void DumpBVHTree(const std::vector<BVHNode>& nodes,
                   size_t node_ind,
                   const std::string& prefix,
                   bool is_last = true);

  void DrawBVHTree(const std::vector<BVHNode>& nodes, size_t node_ind);
};

#endif  // TEAPOT_SCENE_H
