#include "teapot/scene.h"

#include <memory>
#include <unordered_map>

#include "base/vecmath.h"
#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

namespace {

const char vertex_description[] = "p3f;n3f;a3f;t2f";

[[maybe_unused]] void CreateSphere(std::vector<float>& vertices,
                                   std::vector<uint32_t>& indices,
                                   size_t rings,
                                   size_t sectors) {
  float const R = 1. / (float)(rings - 1);
  float const S = 1. / (float)(sectors - 1);

  for (size_t r = 0; r < rings; ++r) {
    for (size_t s = 0; s < sectors; ++s) {
      float y = sin(-PIHALFf + PIf * r * R);
      float x = cos(2 * PIf * s * S) * sin(PIf * r * R);
      float z = sin(2 * PIf * s * S) * sin(PIf * r * R);

      // Position
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      // Normal
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      // Tangent
      vertices.push_back(0);
      vertices.push_back(0);
      vertices.push_back(0);

      // Texture coordinates
      float u = s * S;
      float v = r * R;
      vertices.push_back(u);
      vertices.push_back(v);

      if (r < rings - 1) {
        size_t curRow = r * sectors;
        size_t nextRow = (r + 1) * sectors;
        size_t nextS = (s + 1) % sectors;

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));
        indices.push_back((uint32_t)(curRow + nextS));
      }
    }
  }
}

}  // namespace

Scene::Scene() {
  camera_.Create({0, 0, 0}, -0.06f, 0.1f, 5);
  scene_root_ = std::make_unique<SceneNode>((size_t)-1, "Root");
}

Scene::~Scene() = default;

void Scene::Create() {
  // TestBVH();

  SetVisible(true);

  debug_layer_.CreateRenderResources(Engine::Get().GetRenderer());

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
    return;
  }

  auto source = std::make_unique<ShaderSource>();
  CHECK(source->Load("teapot/pbr.glsl")) << "Could not create ShaderSource";
  shader_id_ = Engine::Get().GetRenderer()->CreateShader(
      std::move(source), vertex_description_, kPrimitive_Triangles, true, false,
      CullMode::kBack);

#if 1
  models_.resize(2);
  {
    // model.LoadObj(Engine::Get().GetRenderer(), shader_id_,
    //                "teapot/viking_room.obj", "", {"teapot/viking_room.png"});
    models_[0].LoadObj(Engine::Get().GetRenderer(), shader_id_,
                       "teapot/buddha.obj", "", {});
    // model.LoadObj(Engine::Get().GetRenderer(), shader_id_,
    //                "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});
    // model.LoadObj(Engine::Get().GetRenderer(), shader_id_,
    //                "teapot/Cerberus_LP.obj", "teapot/Cerberus_LP.mtl",
    //                {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
    //                 "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    for (size_t i = 0; i < 10; ++i) {
      auto node = std::make_unique<SceneNode>(0);
      Quatf q;
      q.Create({0.5f, 0.0f, 0.0f});
      node->setRotation(q);
      node->setPosition({2.2f * i, 0, 0});
      scene_root_->addChild(std::move(node));
    }
  }
  {
    models_[1].LoadObj(Engine::Get().GetRenderer(), shader_id_,
                       "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});

    for (size_t i = 0; i < 3; ++i) {
      auto node = std::make_unique<SceneNode>(1);
      Quatf q;
      q.Create({0.5f, 0.0f, 0.0f});
      node->setRotation(q);
      node->setPosition({2.2f * (10 + i), 0, 0});
      scene_root_->addChild(std::move(node));
    }
  }
#else

  auto& model = models_.emplace_back();
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  CreateSphere(vertices, indices, 32, 32);
  model.CreateMesh(
      Engine::Get().GetRenderer(), shader_id_, vertices, indices,
      // {"teapot/iron-rusted4-basecolor.png", "teapot/iron-rusted4-normal.png",
      //  "teapot/iron-rusted4-metalness.png",
      //  "teapot/iron-rusted4-roughness.png"});
      // {"teapot/greasy-pan-2-albedo.png", "teapot/greasy-pan-2-normal.png",
      //  "teapot/greasy-pan-2-metal.png",
      //  "teapot/greasy-pan-2-roughness.png"});
      // {"teapot/grimy-metal-albedo.png", "teapot/grimy-metal-normal-dx.png",
      //  "teapot/grimy-metal-metalness.png",
      //  "teapot/grimy-metal-roughness.png"});
      // {"teapot/steelplate1_albedo.png", "teapot/steelplate1_normal.png",
      //  "teapot/steelplate1_metallic.png",
      //  "teapot/steelplate1_roughness.png"});
      {"teapot/alien-slime1-albedo.png", "teapot/alien-slime1-normal-dx.png",
       "teapot/alien-slime1-metallic.png",
       "teapot/alien-slime1-roughness.png"});

  // std::vector<eng::MeshObject> bvh_mesh_objects;
  for (size_t i = 0; i < 10; ++i) {
    auto node = std::make_unique<SceneNode>(models_.size() - 1);
    Quatf q;
    q.Create({0.1f, 0.1f, 0.1f});
    node->setRotation(q);
    node->setPosition({2.2f * i, 0, 0});
    scene_root_->addChild(std::move(node));
  }

#endif

  scene_data_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_id_, 1, 0, sizeof(SceneData));
  lights_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(shader_id_, 1, 1,
                                                          sizeof(lights_));
  instances_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_id_, 1, 2, sizeof(InstanceData) * 20);
  scene_dset_ = Engine::Get().GetRenderer()->CreateDescriptorSet(
      shader_id_, 1, {}, {scene_data_ubo_, lights_ubo_, instances_ubo_});

  CreateProjectionMatrix();

  lights_[0].pos = {-15, -4, -15};
  lights_[1].pos = {15, -4, -15};
  lights_[2].pos = {-15, -4, 15};
  lights_[3].pos = {15, -4, 15};
  lights_[0].power = 400.0f;
  lights_[1].power = 400.0f;
  lights_[2].power = 400.0f;
  lights_[3].power = 400.0f;
}

void Scene::Draw(float frame_frac) {
  do {
    std::vector<WorldObject> world_objects =
        scene_root_->GetWorldObjects(models_);

    if (world_objects.empty())
      break;

    bvh_tree_ = BuildBVHTree(world_objects);
    // DumpBVHTree(bvh_tree_, 0, "");

    UpdateViewProjectionMatrix();
    UpdateFrustum();

    auto visible_objects = FrustumCull(bvh_tree_, frustum_);
    DLOG(0) << "FrustumCull: " << visible_objects.size();

    if (visible_objects.empty())
      break;

    std::sort(visible_objects.begin(), visible_objects.end(),
              [](WorldObject& a, WorldObject& b) {
                return a.model_ind < b.model_ind;
              });

    auto draw_list = UpdateInstancesAndGetDrawList(visible_objects);

    UploadSceneData();

    Engine::Get().GetRenderer()->ActivateShader(shader_id_);
    Engine::Get().GetRenderer()->ActivateDescriptorSet(scene_dset_);

    for (auto& draw_call : draw_list) {
      auto [model_ind, first_instance, instance_count] = draw_call;
      models_[model_ind].Draw(instance_count, first_instance);
    }
  } while (false);

#if 1
  DrawBVHTree(bvh_tree_, 0);
  debug_layer_.DrawFrustum(frustum_.planes);
  debug_layer_.DrawMatrix(camera_.GetMatrixMainCam());
  for (auto& instance : instances_)
    debug_layer_.DrawMatrix(instance.model);
#endif

  debug_layer_.Draw(scene_data_.view_projection);
}

void Scene::Update(float delta_time, const Vector2f& angles, float zoom) {
  camera_.Orbit(-angles.y, angles.x, zoom);

  int renderer_type = static_cast<int>(Engine::Get().GetRendererType());

  float label_width = ImGui::CalcTextSize("roughness").x;
  ImGui::SetNextWindowSize(ImVec2(label_width * 3.0f, -1.0f), ImGuiCond_Once);
  if (ImGui::Begin("Teapot", nullptr,
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_NoResize)) {
    ImGui::PushItemWidth(-label_width);
    ImGui::RadioButton("Vulkan", &renderer_type, 1);
    ImGui::SameLine();
    ImGui::RadioButton("OpenGL", &renderer_type, 2);
    ImGui::ColorEdit4("albedo", albedo_.k, ImGuiColorEditFlags_NoAlpha);
    ImGui::SliderFloat("metallic", &metallic_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("roughness", &roughness_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("ambient", &ao_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("light 1", &lights_[0].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 2", &lights_[1].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 3", &lights_[2].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 4", &lights_[3].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("white", &scene_data_.white, 0.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("exposure", &scene_data_.exposure, 0.0f, 20.0f, "%.1f");
  }
  ImGui::End();

  RendererType selected_type = static_cast<RendererType>(renderer_type);
  if (selected_type != Engine::Get().GetRendererType())
    Engine::Get().CreateRenderer(selected_type);

  debug_layer_.Update(delta_time);
}

void Scene::CreateProjectionMatrix() {
  projection_.CreatePerspectiveProjection(
      45, (float)Engine::Get().GetScreenWidth(),
      (float)Engine::Get().GetScreenHeight(), 1.0f, 1000);
}

void Scene::UpdateViewProjectionMatrix() {
  Matrix4f view;
  camera_.GetMatrix().InverseOrthogonal(view);
  view.Multiply(projection_, scene_data_.view_projection);
}

void Scene::UpdateFrustum() {
  Matrix4f view, view_projection;
  camera_.GetMatrixMainCam().InverseOrthogonal(view);
  view.Multiply(projection_, view_projection);
  frustum_.CreateFromMatrix(view_projection);
}

std::vector<std::tuple<size_t, size_t, size_t>>
Scene::UpdateInstancesAndGetDrawList(
    const std::vector<Scene::WorldObject>& objects) {
  instances_.clear();
  std::vector<std::tuple<size_t, size_t, size_t>> draw_list;
  size_t last_model_ind = objects[0].model_ind;
  size_t instance_ind = 0;
  size_t first_instance = 0;
  size_t instance_count = 0;
  for (auto& obj : objects) {
    if (obj.model_ind != last_model_ind) {
      draw_list.emplace_back(last_model_ind, first_instance, instance_count);
      last_model_ind = obj.model_ind;
      first_instance = instance_ind;
      instance_count = 0;
    }
    instances_.emplace_back(obj.transform);
    ++instance_ind;
    ++instance_count;
  }
  draw_list.emplace_back(last_model_ind, first_instance, instance_count);
  return draw_list;
}

void Scene::UploadSceneData() {
  Engine::Get().GetRenderer()->UpdateBuffer(
      instances_ubo_, instances_.data(),
      sizeof(InstanceData) * instances_.size());

  scene_data_.cam_pos = camera_.GetMatrix().Row(3);
  scene_data_.light_dir = {1, 1, 1};
  scene_data_.light_radiance = {1, 1, 1};
  Engine::Get().GetRenderer()->UpdateBuffer(scene_data_ubo_, &scene_data_,
                                            sizeof(scene_data_));

  Engine::Get().GetRenderer()->UpdateBuffer(lights_ubo_, &lights_,
                                            sizeof(lights_));
}

//
// SceneNode
//

Scene::SceneNode::SceneNode(size_t model_ind, const std::string& name)
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
void Scene::SceneNode::addChild(std::unique_ptr<SceneNode> child) {
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
std::unique_ptr<Scene::SceneNode> Scene::SceneNode::removeChild(
    SceneNode* child) {
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

void Scene::SceneNode::setPosition(const Vector3f& position) {
  m_position = position;
  setDirty();
}
void Scene::SceneNode::setRotation(const Quatf& rotation) {
  m_rotation = rotation;
  setDirty();
}
void Scene::SceneNode::setScale(const Vector3f& scale) {
  m_scale = scale;
  setDirty();
}

/**
 * @brief Gets the node's final world transform.
 * Recalculates if dirty.
 */
const Matrix4f& Scene::SceneNode::getWorldTransform() {
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

std::vector<Scene::WorldObject> Scene::SceneNode::GetWorldObjects(
    const std::vector<eng::Model>& models) {
  std::vector<WorldObject> world_objects;
  std::deque<SceneNode*> stack;
  stack.push_back(this);

  while (!stack.empty()) {
    auto* node = stack.back();
    stack.pop_back();

    if (node->model_ind != (size_t)-1) {
      OBBf obb{node->getWorldTransform(), models[node->model_ind].GetExtents()};
      world_objects.emplace_back(node->model_ind, obb,
                                 node->getWorldTransform());
    }

    for (auto& child : node->m_children)
      stack.push_back(child.get());
  }

  return world_objects;
}

void Scene::SceneNode::setDirty() {
  if (m_isDirty)
    return;  // Already dirty, no need to propagate

  m_isDirty = true;
  for (auto& child : m_children) {
    child->setDirty();
  }
}

std::vector<Scene::BVHNode> Scene::BuildBVHTree(
    std::vector<WorldObject> objects) {
  if (objects.empty())
    return {};

  std::vector<BVHNode> bvh_nodes(2 * objects.size() - 1);
  size_t node_ind_last = 0;  // std::make_unique<BVHNode>();

  // Create stack for depth-first traversal and start the process with the root
  // node using all objects.
  std::deque<std::tuple<size_t, std::span<WorldObject>>> stack;
  std::span objects_view(objects.data(), objects.size());
  stack.push_back(std::make_tuple(node_ind_last, std::move(objects_view)));

  while (!stack.empty()) {
    auto [node_ind, node_objects] = std::move(stack.back());
    stack.pop_back();

    // If only one object remains, this is a leaf.
    if (node_objects.size() == 1) {
      bvh_nodes[node_ind].object = node_objects[0];
      continue;
    }

    // Calculate the combined AABB for all node_objects in this job.
    for (auto& obj : node_objects) {
      AABBf aabb;
      obj.obb.GetBoundBox(aabb);
      bvh_nodes[node_ind].aabb.Expand(aabb);
    }

    // Find the longest axis of the combined AABB to split along.
    Vector3f extent =
        bvh_nodes[node_ind].aabb.max - bvh_nodes[node_ind].aabb.min;
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;
    if (extent.z > extent.y)
      axis = 2;

    // Sort node_objects along the chosen axis based on their center point.
    if (axis == 0) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.x < b.obb.center.x;
                });
    } else if (axis == 1) {
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.y < b.obb.center.y;
                });
    } else {  // axis == 2
      std::sort(node_objects.begin(), node_objects.end(),
                [](WorldObject& a, WorldObject& b) {
                  return a.obb.center.z < b.obb.center.z;
                });
    }

    // Split the objects into two halves
    size_t mid = node_objects.size() / 2;
    std::span<WorldObject> left_objects(node_objects.begin(),
                                        node_objects.begin() + mid);
    std::span<WorldObject> right_objects(node_objects.begin() + mid,
                                         node_objects.end());

    // Create the child nodes.
    bvh_nodes[node_ind].left = ++node_ind_last;
    bvh_nodes[node_ind].right = ++node_ind_last;

    // Push the children onto the stack.
    stack.push_back({bvh_nodes[node_ind].left, std::move(right_objects)});
    stack.push_back({bvh_nodes[node_ind].right, std::move(left_objects)});
  }

  return bvh_nodes;
}

std::vector<Scene::WorldObject> Scene::FrustumCull(
    const std::vector<BVHNode>& nodes,
    const Frustumf& frustum) {
  std::vector<WorldObject> visible_objects;
  if (nodes.empty())
    return visible_objects;

  // Create stack for depth-first traversal and start the process with the root
  // of the BVH tree.
  std::deque<size_t> stack;
  stack.push_back(0);

  while (!stack.empty()) {
    size_t node_ind = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (nodes[node_ind].IsLeaf()) {
      if (frustum.Intersects(nodes[node_ind].object.obb,
                             nodes[node_ind].object.transform))
        visible_objects.push_back(nodes[node_ind].object);
      continue;
    } else if (!frustum.Intersects(nodes[node_ind].aabb)) {
      continue;
    }

    // The internal node passed tests, check its children
    if (nodes[node_ind].left)
      stack.push_back(nodes[node_ind].left);
    if (nodes[node_ind].right)
      stack.push_back(nodes[node_ind].right);
  }

  return visible_objects;
}

void Scene::DumpBVHTree(const std::vector<BVHNode>& nodes,
                        size_t node_ind,
                        const std::string& prefix,
                        bool is_last) {
  if (nodes.empty())
    return;

  std::ostringstream out;

  // Print the current node's line
  out << prefix;
  out << (is_last ? "└──" : "├──");

  AABBf aabb;

  // Print node details
  if (nodes[node_ind].IsLeaf()) {
    out << "[Leaf] model_ind: " << nodes[node_ind].object.model_ind << " ";
    nodes[node_ind].object.obb.GetBoundBox(aabb);
  } else {
    out << "[Internal] ";
    aabb = nodes[node_ind].aabb;
  }

  // Print bounding box info
  Vector3f center = (aabb.max + aabb.min) * 0.5f;
  Vector3f extent = (aabb.max - aabb.min) * 0.5f;
  out << "Center: " << center.ToString() << " Extent: " << extent.ToString();
  DLOG(0) << out.str();

  // Prepare the prefix for the children
  std::string child_prefix = prefix + (is_last ? "    " : "│   ");

  // Recurse for children (if they exist)
  if (!nodes[node_ind].IsLeaf()) {
    // The right child is always the "last" one for its parent
    DumpBVHTree(nodes, nodes[node_ind].left, child_prefix, false);
    DumpBVHTree(nodes, nodes[node_ind].right, child_prefix, true);
  }
}

void Scene::DrawBVHTree(const std::vector<BVHNode>& nodes, size_t node_ind) {
  if (nodes.empty())
    return;

  if (nodes[node_ind].IsLeaf()) {
    debug_layer_.DrawObb(nodes[node_ind].object.obb, {1, 1, 0});
  } else {
    debug_layer_.DrawAabb(nodes[node_ind].aabb, {1, 0, 1});
  }

  // Recurse for children (if they exist)
  if (!nodes[node_ind].IsLeaf()) {
    // The right child is always the "last" one for its parent
    DrawBVHTree(nodes, nodes[node_ind].left);
    DrawBVHTree(nodes, nodes[node_ind].right);
  }
}
