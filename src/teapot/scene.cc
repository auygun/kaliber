#include "teapot/scene.h"

#include <algorithm>
#include <memory>
#include <span>
#include <tuple>

#include "base/log.h"
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
  // Create the root node at index 0
  Node root_node;
  root_node.is_active = true;
  root_node.name = "Root";

  nodes_.push_back(root_node);
  root_index_ = 0;

  // Free list is initially empty
  free_list_head_ = NO_NODE;

  camera_.Create({0, 0, 0}, -0.06f, 0.1f, 5);
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
      auto node_index = CreateNode(root_index_, 0);
      Quatf q;
      q.Create({0.5f, 0.0f, 0.0f});
      SetRotation(node_index, q);
      SetPosition(node_index, {2.2f * i, 0, 0});
    }
  }
  {
    models_[1].LoadObj(Engine::Get().GetRenderer(), shader_id_,
                       "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});

    for (size_t i = 0; i < 3; ++i) {
      auto node_index = CreateNode(root_index_, 1);
      Quatf q;
      q.Create({0.5f, 0.0f, 0.0f});
      SetRotation(node_index, q);
      SetPosition(node_index, {2.2f * (10 + i), 0, 0});
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
    auto node = std::make_unique<Node>(models_.size() - 1);
    Quatf q;
    q.Create({0.1f, 0.1f, 0.1f});
    node->SetRotation(q);
    node->SetPosition({2.2f * i, 0, 0});
    scene_root_.AddChild(std::move(node));
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
  UpdateViewProjectionMatrix();
  UpdateFrustum();

  instances_.clear();
  do {
    std::vector<WorldObject> world_objects = GetSceneObjects();
    if (world_objects.empty())
      break;

    bvh_tree_ = BuildBVHTree(world_objects);

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
  // DumpBVHTree(bvh_tree_, 0, "");
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
  DCHECK(instances_.empty());
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
// Node
//

size_t Scene::AllocateNode() {
  if (free_list_head_ != NO_NODE) {
    // --- Reuse a node from the free list ---
    size_t new_index = free_list_head_;
    Node& new_node = nodes_[new_index];
    free_list_head_ = new_node.next_sibling;  // Pop from free list

    // Reset the node to a default state
    new_node = Node{};
    new_node.is_active = true;
    return new_index;
  } else {
    // --- No free nodes, create a new one ---
    size_t new_index = nodes_.size();
    Node new_node;
    new_node.is_active = true;
    nodes_.push_back(new_node);
    return new_index;
  }
}

void Scene::FreeNode(size_t index) {
  if (index == root_index_ || index >= nodes_.size() ||
      !nodes_[index].is_active) {
    // Cannot free the root node or an already-inactive node
    return;
  }

  Node& node = nodes_[index];
  node.is_active = false;

  // Add this node's index to the front of the free list
  node.next_sibling = free_list_head_;
  free_list_head_ = index;
}

size_t Scene::CreateNode(size_t parent_index,
                         size_t model_ind,
                         const std::string& name) {
  assert(parent_index < nodes_.size() && nodes_[parent_index].is_active &&
         "Invalid parent index");

  size_t new_index = AllocateNode();

  Node& node = nodes_[new_index];
  node.name = name;
  node.model_ind = model_ind;

  // Attach to parent
  AttachToParent(parent_index, new_index);

  return new_index;
}

void Scene::DeleteNode(size_t index) {
  if (index == root_index_ || index >= nodes_.size() ||
      !nodes_[index].is_active) {
    return;  // Cannot delete root or invalid node
  }

  Node& node = nodes_[index];

  // --- 1. Recursively delete all children ---
  // We must iterate carefully as the child list will be modified.
  size_t child_index = node.first_child;
  while (child_index != NO_NODE) {
    // Get the *next* sibling *before* deleting the current child,
    // as DeleteNode will modify the 'next_sibling' field when it's freed.
    size_t next_child = nodes_[child_index].next_sibling;
    DeleteNode(child_index);  // Recursive call
    child_index = next_child;
  }

  // --- 2. Detach from parent ---
  DetachFromParent(index);

  // --- 3. Return this node to the free list ---
  FreeNode(index);
}

void Scene::SetParent(size_t parent_index, size_t child_index) {
  assert(parent_index < nodes_.size() && nodes_[parent_index].is_active &&
         "Invalid parent index");
  assert(child_index < nodes_.size() && nodes_[child_index].is_active &&
         "Invalid child index");
  assert(child_index != root_index_ && "Cannot re-parent the root node");

  if (nodes_[child_index].parent == parent_index) {
    return;  // Already attached to this parent
  }

  // 1. Detach from old parent
  DetachFromParent(child_index);

  // 2. Attach to new parent
  AttachToParent(parent_index, child_index);
}

void Scene::AttachToParent(size_t parent_index, size_t child_index) {
  Node& parent = nodes_[parent_index];
  Node& child = nodes_[child_index];

  child.parent = parent_index;

  // Insert at the front of the parent's child list
  size_t old_first_child = parent.first_child;
  parent.first_child = child_index;

  child.next_sibling = old_first_child;
  child.prev_sibling = NO_NODE;

  if (old_first_child != NO_NODE) {
    nodes_[old_first_child].prev_sibling = child_index;
  }
}

void Scene::DetachFromParent(size_t node_index) {
  assert(node_index < nodes_.size() && "Invalid node index");
  Node& node = nodes_[node_index];
  size_t parent_index = node.parent;

  if (parent_index == NO_NODE) {
    return;  // Already detached (or is the root)
  }

  assert(parent_index < nodes_.size() && "Invalid parent index on node");
  Node& parent = nodes_[parent_index];
  size_t prev_sib = node.prev_sibling;
  size_t next_sib = node.next_sibling;

  // Unlink from the doubly-linked sibling list
  if (prev_sib != NO_NODE) {
    nodes_[prev_sib].next_sibling = next_sib;
  } else {
    // This was the first child, so update parent's pointer
    parent.first_child = next_sib;
  }

  if (next_sib != NO_NODE) {
    nodes_[next_sib].prev_sibling = prev_sib;
  }

  // Clear the node's own hierarchy links
  node.parent = NO_NODE;
  node.next_sibling = NO_NODE;
  node.prev_sibling = NO_NODE;
}

void Scene::SetPosition(size_t index, const base::Vector3f& p) {
  assert(index < nodes_.size() && nodes_[index].is_active &&
         "Invalid node index");
  nodes_[index].position = p;
  SetDirty(index);
}

void Scene::SetRotation(size_t index, const base::Quatf& r) {
  assert(index < nodes_.size() && nodes_[index].is_active &&
         "Invalid node index");
  nodes_[index].rotation = r;
  SetDirty(index);
}

void Scene::SetScale(size_t index, float s) {
  assert(index < nodes_.size() && nodes_[index].is_active &&
         "Invalid node index");
  nodes_[index].scale = s;
  SetDirty(index);
}

void Scene::SetDirty(size_t index) {
  assert(index < nodes_.size() && "Invalid node index");
  Node& node = nodes_[index];
  if (node.is_dirty) {
    return;  // Already dirty, no need to propagate
  }

  node.is_dirty = true;

  // Propagate dirty flag to all children
  size_t child_index = node.first_child;
  while (child_index != NO_NODE) {
    SetDirty(child_index);  // Recursive call
    child_index = nodes_[child_index].next_sibling;
  }
}

const base::Matrix4f& Scene::GetWorldTransform(size_t index) {
  assert(index < nodes_.size() && nodes_[index].is_active &&
         "Invalid node index");
  Node& node = nodes_[index];

  if (node.is_dirty) {
    // 1. Recalculate local transform
    node.local_transform.Create(node.rotation, node.position);
    node.local_transform.Multiply(node.scale);

    // 2. Combine with parent's world transform
    if (node.parent != NO_NODE) {
      // Recursively update parent first
      const base::Matrix4f& parentWorld = GetWorldTransform(node.parent);
      parentWorld.Multiply(node.local_transform, node.world_transform);
    } else {
      node.world_transform = node.local_transform;  // This is the root node
    }

    // 3. Mark as clean
    node.is_dirty = false;
  }

  return node.world_transform;
}

std::vector<Scene::WorldObject> Scene::GetSceneObjects() {
  std::vector<WorldObject> world_objects;
  std::deque<size_t> stack;
  stack.push_back(root_index_);

  while (!stack.empty()) {
    size_t index = stack.back();
    stack.pop_back();

    if (nodes_[index].model_ind != (size_t)-1) {
      OBBf obb{GetWorldTransform(index),
               models_[nodes_[index].model_ind].GetExtents()};
      world_objects.emplace_back(nodes_[index].model_ind, obb,
                                 GetWorldTransform(index));
    }

    size_t child_index = nodes_[index].first_child;
    while (child_index != NO_NODE) {
      stack.push_back(child_index);
      child_index = nodes_[child_index].next_sibling;
    }
  }

  return world_objects;
}

std::vector<Scene::BVHNode> Scene::BuildBVHTree(
    std::vector<WorldObject> objects) {
  if (objects.empty())
    return {};

  std::vector<BVHNode> bvh_nodes(2 * objects.size() - 1);
  size_t node_ind_last = 0;

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
