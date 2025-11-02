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

namespace eng {

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
}

Scene::~Scene() {
  debug_layer_.Shutdown();

  renderer_->DestroyDescriptorSet(scene_dset_);
  renderer_->DestroyBuffer(scene_data_ubo_);
  renderer_->DestroyBuffer(lights_ubo_);
  renderer_->DestroyBuffer(instances_ubo_);

  renderer_->DestroyShader(shader_id_);
}

void Scene::Create(Renderer* renderer) {
  renderer_ = renderer;

  // TestBVH();

  debug_layer_.CreateRenderResources(renderer_);

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
    return;
  }

  auto source = std::make_unique<ShaderSource>();
  CHECK(source->Load("teapot/pbr.glsl")) << "Could not create ShaderSource";
  shader_id_ = renderer_->CreateShader(std::move(source), vertex_description_,
                                       kPrimitive_Triangles, true, false,
                                       CullMode::kBack);

  registry_.CreatePool<CoreDataComponent>();

  root_entity_ = registry_.CreateEntity();
  registry_.AddComponent(root_entity_, CoreDataComponent{.name{"root"}})
      .local_transform.Create(Quatf({0.5f, 0.0f, 0.0f}), {0, 0, 0});

#if 1
  Entity parent = root_entity_;
  models_.resize(4);
  {
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/viking_room.obj", "", {"teapot/viking_room.png"});
    models_[0].LoadObj(renderer_, shader_id_, "teapot/buddha.obj", "", {});
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/Cerberus_LP.obj", "teapot/Cerberus_LP.mtl",
    //                {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
    //                 "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    for (size_t i = 0; i < 10; ++i) {
      Entity entity = registry_.CreateEntity();
      CoreDataComponent core_data{
          .name{"model"}, .parent{parent}, .model_index{0}};
      core_data.local_transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, 0, 0});
      // core_data.local_transform.M_x_RotY(0.01);
      registry_.AddComponent(entity, core_data);
      registry_.GetComponent<CoreDataComponent>(parent).AddChild(entity);
      parent = entity;
    }
  }
  {
    models_[1].LoadObj(renderer_, shader_id_, "teapot/sportsCar.obj",
                       "teapot/sportsCar.mtl", {});

    for (size_t i = 0; i < 3; ++i) {
      Entity entity = registry_.CreateEntity();
      CoreDataComponent core_data{
          .name{"model"}, .parent{parent}, .model_index{1}};
      core_data.local_transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, 0, 0});
      registry_.AddComponent(entity, core_data);
      registry_.GetComponent<CoreDataComponent>(parent).AddChild(entity);
      parent = entity;
    }
  }
  {
    models_[2].LoadObj(renderer_, shader_id_, "teapot/Cerberus_LP.obj",
                       "teapot/Cerberus_LP.mtl",
                       {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
                        "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    for (size_t i = 0; i < 1; ++i) {
      Entity entity = registry_.CreateEntity();
      CoreDataComponent core_data{
          .name{"model"}, .parent{root_entity_}, .model_index{2}};
      core_data.local_transform.Create(Quatf({0.0f, 0.0f, 0.0f}),
                                       {200.0f, -100.0f, 0.0f});
      core_data.local_transform.Multiply(0.05f);
      registry_.AddComponent(entity, core_data);
      registry_.GetComponent<CoreDataComponent>(root_entity_).AddChild(entity);
      // parent = entity;
    }
  }
  // #else

  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  CreateSphere(vertices, indices, 32, 32);
  models_[3].CreateMesh(
      renderer_, shader_id_, vertices, indices,
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

  for (size_t i = 0; i < 10; ++i) {
    Entity entity = registry_.CreateEntity();
    CoreDataComponent core_data{
        .name{"model"}, .parent{parent}, .model_index{models_.size() - 1}};
    core_data.local_transform.Create(Quatf({0.0f, 0.0f, 0.1f}), {2.2f, 0, 0});
    registry_.AddComponent(entity, core_data);
    registry_.GetComponent<CoreDataComponent>(parent).AddChild(entity);
    parent = entity;
  }

#endif

  scene_data_ubo_ =
      renderer_->CreateBuffer(shader_id_, 1, 0, sizeof(SceneData));
  lights_ubo_ = renderer_->CreateBuffer(shader_id_, 1, 1, sizeof(lights_));
  instances_ubo_ =
      renderer_->CreateBuffer(shader_id_, 1, 2, sizeof(InstanceData) * 100);
  scene_dset_ = renderer_->CreateDescriptorSet(
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

void Scene::Render(float frame_frac) {
  UpdateViewProjectionMatrix();
  UpdateFrustum();

  instances_.clear();

  do {
    std::vector<WorldObject> world_objects;
    // Skip root entity and iterate through.
    for (auto [entity, core_data] : registry_.View<CoreDataComponent>(1)) {
      OBBf obb{GetWorldTransform(core_data),
               models_[core_data.model_index].GetExtents()};
      world_objects.emplace_back(entity, core_data.model_index, obb,
                                 GetWorldTransform(core_data));
    }
    if (world_objects.empty())
      break;

    bvh_tree_ = BuildBVHTree(std::move(world_objects));

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

    renderer_->ActivateShader(shader_id_);
    renderer_->ActivateDescriptorSet(scene_dset_);

    for (auto& draw_call : draw_list) {
      auto [model_ind, first_instance, instance_count] = draw_call;
      models_[model_ind].Draw(instance_count, first_instance);
    }
  } while (false);

#if 1
  if (show_bounding_volumes_) {
    // DumpBVHTree(bvh_tree_, 0, "");
    DrawBVHTree(bvh_tree_, 0);
    debug_layer_.DrawFrustum(frustum_);
    debug_layer_.DrawMatrix(camera_.GetMatrixMainCam());
    for (auto& instance : instances_)
      debug_layer_.DrawMatrix(instance.model);
  }
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
    ImGui::Checkbox("Volumes", &show_bounding_volumes_);
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

void Scene::OnClick(const base::Vector2f& pos) {
  Rayf ray = CreateRayFromScreen(pos.x, pos.y);
  debug_layer_.DrawVector(ray.origin, ray.direction, Vector3f{1.0f}, 1, true);
  selected_entity_ = SelectEntity(bvh_tree_, ray);
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
  renderer_->UpdateBuffer(instances_ubo_, instances_.data(),
                          sizeof(InstanceData) * instances_.size());

  scene_data_.cam_pos = camera_.GetMatrix().Row(3);
  scene_data_.light_dir = {1, 1, 1};
  scene_data_.light_radiance = {1, 1, 1};
  renderer_->UpdateBuffer(scene_data_ubo_, &scene_data_, sizeof(scene_data_));

  renderer_->UpdateBuffer(lights_ubo_, &lights_, sizeof(lights_));
}

void Scene::SetDirty(CoreDataComponent& core_data) {
  if (core_data.is_dirty)
    return;

  auto* pool = registry_.GetPool<CoreDataComponent>();

  std::deque<Entity> stack;
  for (Entity child : core_data.children)
    stack.push_back(child);

  while (!stack.empty()) {
    Entity entity = stack.back();
    stack.pop_back();

    DCHECK(pool->Has(entity));
    auto& child_core_data = pool->Get(entity);
    child_core_data.is_dirty = true;
    for (Entity child : child_core_data.children)
      stack.push_back(child);
  }

  core_data.is_dirty = true;
}

const base::Matrix4f& Scene::GetWorldTransform(CoreDataComponent& core_data) {
  if (core_data.is_dirty) {
    auto* pool = registry_.GetPool<CoreDataComponent>();

    std::deque<Entity> stack;
    CoreDataComponent* parent_core_data = &core_data;
    while (parent_core_data->parent != NULL_ENTITY) {
      DCHECK(pool->Has(parent_core_data->parent));
      stack.push_back(parent_core_data->parent);
      parent_core_data = &pool->Get(parent_core_data->parent);
    }

    base::Matrix4f world_transform{1};
    while (!stack.empty()) {
      Entity entity = stack.back();
      stack.pop_back();

      auto& parent_core_data = pool->Get(entity);
      world_transform.Multiply(parent_core_data.local_transform,
                               parent_core_data.world_transform);
      world_transform = parent_core_data.world_transform;
      parent_core_data.is_dirty = false;
    }

    world_transform.Multiply(core_data.local_transform,
                             core_data.world_transform);
    core_data.is_dirty = false;
  }

  return core_data.world_transform;
}

void Scene::SetParent(Entity entity, Entity new_parent) {
  DCHECK(entity != NULL_ENTITY);

  auto* pool = registry_.GetPool<CoreDataComponent>();

  // Remove the entity from its parent's child list.
  DCHECK(pool->Has(entity));
  auto& core_data = pool->Get(entity);
  if (core_data.parent != NULL_ENTITY) {
    DCHECK(pool->Has(core_data.parent));
    pool->Get(core_data.parent).RemoveChild(entity);
  }

  // Set the new parent and add to its child list.
  core_data.parent = new_parent;
  if (new_parent != NULL_ENTITY) {
    DCHECK(pool->Has(new_parent));
    pool->Get(new_parent).AddChild(entity);
  }

  SetDirty(core_data);
}

void Scene::DestroyEntityAndChildren(Entity entity) {
  DCHECK(entity != NULL_ENTITY);

  auto* pool = registry_.GetPool<CoreDataComponent>();

  // Remove the entity from its parent's child list.
  DCHECK(pool->Has(entity));
  auto& core_data = pool->Get(entity);
  if (core_data.parent != NULL_ENTITY) {
    DCHECK(pool->Has(core_data.parent));
    pool->Get(core_data.parent).RemoveChild(entity);
  }

  // Cascade delete all children.
  std::deque<Entity> stack;
  stack.push_back(entity);
  while (!stack.empty()) {
    Entity entity_to_destroy = stack.back();
    stack.pop_back();

    DCHECK(pool->Has(entity_to_destroy));
    auto& core_data = pool->Get(entity_to_destroy);
    for (Entity child : core_data.children)
      stack.push_back(child);
    registry_.DestroyEntity(entity_to_destroy);
  }
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

    // Calculate the combined AABB for all node_objects in this branch.
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
    Vector3f color{1, 1, 0};
    if (nodes[node_ind].object.entity == selected_entity_)
      color = {0, 1, 1};
    debug_layer_.DrawObb(nodes[node_ind].object.obb, color);
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

Rayf Scene::CreateRayFromScreen(float screen_x, float screen_y) {
  // Convert Screen Coords to Normalized Device Coords (NDC) [-1, 1]
  float ndc_x = (2.0f * screen_x) / Engine::Get().GetScreenWidth() - 1.0f;
  float ndc_y = 1.0f - (2.0f * screen_y) / Engine::Get().GetScreenHeight();

  // Unproject the points from clip space to world space.
  Matrix4f inv_view_proj = scene_data_.view_projection;
  inv_view_proj.Inverse();
  Vector3f origin = Vector3f(ndc_x, ndc_y, 0.0f) * inv_view_proj;
  Vector3f dir = Vector3f(ndc_x, ndc_y, 1.0f) * inv_view_proj;
  dir.Normalize();

  return Rayf{origin, dir};
}

// Selects an entity by casting a ray.
Entity Scene::SelectEntity(const std::vector<BVHNode>& nodes, const Rayf& ray) {
  Entity selected_entity = NULL_ENTITY;
  float closest_distance{std::numeric_limits<float>::max()};

  std::deque<size_t> stack;
  stack.push_back(0);

  while (!stack.empty()) {
    size_t node_ind = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (nodes[node_ind].IsLeaf()) {
      float distance = nodes[node_ind].object.obb.IntersectRay(ray);
      if (distance >= 0.0f && distance < closest_distance) {
        closest_distance = distance;
        selected_entity = nodes[node_ind].object.entity;
      }
      continue;
    } else if (nodes[node_ind].aabb.IntersectRay(ray) < 0) {
      continue;
    }

    // The internal node passed tests, check its children
    if (nodes[node_ind].left)
      stack.push_back(nodes[node_ind].left);
    if (nodes[node_ind].right)
      stack.push_back(nodes[node_ind].right);
  }

  return selected_entity;
}

void Scene::CoreDataComponent::AddChild(Entity child_entity) {
  children.push_back(child_entity);
}

void Scene::CoreDataComponent::RemoveChild(Entity child_entity) {
  children.erase(std::remove(children.begin(), children.end(), child_entity),
                 children.end());
}

}  // namespace eng
