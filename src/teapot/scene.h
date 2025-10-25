#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <vector>

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
    size_t id = -1;
    size_t model_ind = (size_t)-1;
    base::OBBf obb;
    base::Matrix4f transform{1};
  };

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
     * @return std::unique_ptr<SceneNode> to the detached child, or nullptr if
     * not found.
     */
    std::unique_ptr<SceneNode> removeChild(SceneNode* child);

    // --- Transform Management ---

    void setPosition(const base::Vector3f& position);
    void setRotation(const base::Quatf& rotation);
    void setScale(const base::Vector3f& scale);

    /**
     * @brief Gets the node's final world transform.
     * Recalculates if dirty.
     */
    const base::Matrix4f& getWorldTransform();

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
    base::Vector3f m_position{0};
    base::Quatf m_rotation{0, 0, 0, 1};
    base::Vector3f m_scale{1.0f};

    // --- Cached Matrices ---
    base::Matrix4f m_localTransform{1};
    base::Matrix4f m_worldTransform{1};
    bool m_isDirty = true;  // Start dirty to force initial calculation

    // --- Hierarchy ---
    std::string m_name;
    SceneNode* m_parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> m_children;

    size_t model_ind = (size_t)-1;
  };

  struct BVHNode {
    // Bounding volume for the node
    base::AABBf aabb;

    // Tree structure
    size_t left = (size_t)-1;
    size_t right = (size_t)-1;

    // size_t object_ind = (size_t)-1;  // If not -1, this is a leaf node
    WorldObject object;

    bool isLeaf() const { return object.id != (size_t)-1; }
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

  eng::DebugLayer debug_layer_;

  std::unique_ptr<SceneNode> scene_root_;
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
