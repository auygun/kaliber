#include "engine/world.h"

#include <algorithm>
#include <memory>
#include <span>
#include <tuple>

#include "base/log.h"
#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/renderer/renderer.h"

using namespace base;

namespace eng {

namespace {

const char vertex_description[] = "p3f;n3f;a4f;t2f";

[[maybe_unused]] void CreateSphere(std::vector<Model::Vertex>& vertices,
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
      float u = s * S;
      float v = r * R;

      Model::Vertex vert{};
      vert.position = {x, y, z};
      vert.normal = {x, y, z};
      vert.uv[0] = u;
      vert.uv[1] = v;
      vertices.push_back(vert);

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

World::World() = default;

World::~World() {
  debug_layer_.Shutdown();

  renderer_->DestroyDescriptorSet(scene_dset_);
  renderer_->DestroyBuffer(scene_data_ubo_);
  renderer_->DestroyBuffer(lights_ubo_);
  renderer_->DestroyBuffer(instances_ubo_);

  renderer_->DestroyShader(shader_id_);
}

void World::Create(Renderer* renderer) {
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

  // Cache pointers to the pools.
  scene_node_pool_ = registry_.GetPool<SceneNodeComponent>();
  world_transform_pool_ = registry_.GetPool<WorldTransformComponent>();
  local_transform_pool_ = registry_.GetPool<LocalTransformComponent>();
  dirty_tag_pool_ = registry_.GetPool<WorldTransformDirtyTag>();
  world_bounds_pool_ = registry_.GetPool<WorldBoundsComponent>();
  model_pool_ = registry_.GetPool<ModelComponent>();

  // Create global resources and cache the pointers.
  render_context_ = &registry_.GetSingletonComponent<RenderContext>();

  // Create the root entity of the scene-graph.
  root_entity_ = registry_.CreateEntity();
  registry_.AddComponent(root_entity_, SceneNodeComponent{.name{"root"}});
  registry_.AddComponent(root_entity_, WorldTransformComponent{});
  registry_.AddComponent(root_entity_, LocalTransformComponent{})
      .transform.Create(Quatf({0.5f, 0.0f, 0.0f}), {0, 0, 0});
  OnHierarchyChanged(root_entity_, 0);
  registry_.AddComponent(root_entity_, WorldTransformDirtyTag{});

  // // Create camera entity
  // {
  //   Entity entity = registry_.CreateEntity();
  //   registry_.AddComponent(entity, SceneNodeComponent{.name{"cam"}});
  //   registry_.AddComponent(entity, WorldTransformComponent{});
  //   registry_.AddComponent(entity, LocalTransformComponent{});
  //   registry_.AddComponent(entity, FlyCameraComponent{});
  //   registry_.AddComponent(entity, PrimaryCameraTag{});
  //   registry_.AddComponent(
  //       entity, CameraComponent{
  //                   .fov = 45.0f, .near_plane = 1.0f, .far_plane = 1000.0f});
  //   SetParent(entity, NULL_ENTITY);
  // }

#if 1
  Entity parent = root_entity_;
  models_.resize(7);
  {
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/viking_room.obj", "", {"teapot/viking_room.png"});
    // models_[0].LoadObj(renderer_, shader_id_, "teapot/buddha.obj", "", {});
    models_[0].LoadGLTF(renderer_, shader_id_, "teapot/Cube.gltf");
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});
    // model.LoadObj(renderer_, shader_id_,
    //                "teapot/Cerberus_LP.obj", "teapot/Cerberus_LP.mtl",
    //                {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
    //                 "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    for (size_t i = 0; i < 10; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, 0, 0});
      Entity entity = NewEntity(parent, 0, transform);
      parent = entity;
    }
  }
  {
    models_[1].LoadObj(renderer_, shader_id_, "teapot/sportsCar.obj",
                       "teapot/sportsCar.mtl", {});

    for (size_t i = 0; i < 3; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, 0, 0});
      Entity entity = NewEntity(parent, 1, transform);
      parent = entity;
    }
  }
  {
    models_[2].LoadObj(renderer_, shader_id_, "teapot/Cerberus_LP.obj",
                       "teapot/Cerberus_LP.mtl",
                       {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
                        "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    for (size_t i = 0; i < 1; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}),
                       Vector3f{200.0f, -100.0f, 0.0f});
      transform.Multiply(0.05f);
      NewEntity(root_entity_, 2, transform);
      // parent = entity;
    }
  }
  {
    models_[3].LoadGLTF(renderer_, shader_id_, "teapot/WaterBottle.glb");

    for (size_t i = 0; i < 1; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0.0f, -0.5f, 0.0f});
      transform.Multiply(10.0f);
      NewEntity(root_entity_, 3, transform);
      // parent = entity;
    }
  }
  {
    models_[4].LoadGLTF(renderer_, shader_id_, "teapot/Avocado.glb");

    for (size_t i = 0; i < 1; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0.0f, -0.5f, 0.2f});
      transform.Multiply(10.0f);
      NewEntity(root_entity_, 4, transform);
      // parent = entity;
    }
  }
  {
    models_[5].LoadGLTF(renderer_, shader_id_, "teapot/BarramundiFish.glb");

    for (size_t i = 0; i < 1; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0.0f, -0.5f, 0.7f});
      transform.Multiply(10.0f);
      NewEntity(root_entity_, 5, transform);
      // parent = entity;
    }
  }
  // #else

  std::vector<Model::Vertex> vertices;
  std::vector<uint32_t> indices;
  CreateSphere(vertices, indices, 32, 32);
  models_[6].CreateMesh(
      renderer_, shader_id_, std::move(vertices), std::move(indices),
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
    Matrix4f transform;
    transform.Create(Quatf({0.0f, 0.0f, 0.1f}), {2.2f, 0, 0});
    Entity entity = NewEntity(parent, models_.size() - 1, transform);
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

  lights_[0].pos = {-15, -4, -15};
  lights_[1].pos = {15, -4, -15};
  lights_[2].pos = {-15, -4, 15};
  lights_[3].pos = {15, -4, 15};
  lights_[0].power = 400.0f;
  lights_[1].power = 400.0f;
  lights_[2].power = 400.0f;
  lights_[3].power = 400.0f;
}

Entity World::CreateSceneNode(std::string_view name,
                              Entity parent,
                              const Matrix4f& transform) {
  Entity entity = registry_.CreateEntity();
  SceneNodeComponent scene_node{};
  name.copy(scene_node.name, 7, 0);
  registry_.AddComponent(entity, scene_node);
  registry_.AddComponent(entity, WorldTransformComponent{});
  registry_.AddComponent(entity, LocalTransformComponent{transform});
  SetParent(entity, parent);
  return entity;
}

Entity World::NewEntity(Entity parent,
                        uint32_t model_index,
                        const Matrix4f& transform) {
  Entity entity = registry_.CreateEntity();
  registry_.AddComponent(entity, SceneNodeComponent{.name{"model"}});
  registry_.AddComponent(entity, WorldTransformComponent{});
  registry_.AddComponent(entity, LocalTransformComponent{transform});
  registry_.AddComponent(entity, WorldBoundsComponent{});
  registry_.AddComponent(
      entity, ModelComponent{model_index, models_[model_index].GetExtents()});
  SetParent(entity, parent);
  return entity;
}

void World::SceneGraphUpdate() {
  UpdateWoldTransforms();
  UpdateWorldBounds();
  dirty_tag_pool_->RemoveAll();
}

void World::Render(float frame_frac) {
  UpdateRenderContext();
  frustum_.CreateFromMatrix(render_context_->view_proj);

  // Build the flat list used for building the BVH tree.
  std::vector<BVHBuildItem> flat_list;
  flat_list.reserve(world_bounds_pool_->GetSize());
  for (auto [entity, world_bounds] : registry_.View<WorldBoundsComponent>()) {
    AABBf aabb;
    world_bounds.obb.GetBoundBox(aabb);
    flat_list.emplace_back(entity, aabb, world_bounds.obb.center);
  }

  BuildBVHTree(std::move(flat_list));

  auto sort_list = FrustumCull(frustum_);
  DLOG(0) << "FrustumCull: " << sort_list.size();
  if (!sort_list.empty()) {
    std::sort(sort_list.begin(), sort_list.end());

    auto draw_list = BuildDrawList(std::move(sort_list));

    UploadSceneData();

    renderer_->ActivateShader(shader_id_);
    renderer_->ActivateDescriptorSet(scene_dset_);

    for (auto& draw_call : draw_list) {
      auto [model_index, first_instance, instance_count] = draw_call;
      models_[model_index].Draw(instance_count, first_instance);
    }
  }

#if 1
  if (render_context_->show_bounding_volumes) {
    // DumpBVHTree(bvh_tree_, 0, "");
    DrawBVHTree(bvh_tree_, 0);
    debug_layer_.DrawFrustum(frustum_);
    Matrix4f camera_mat;
    render_context_->view.InverseOrthogonal(camera_mat);
    debug_layer_.DrawMatrix(camera_mat);
    for (auto& instance : instances_)
      debug_layer_.DrawMatrix(instance.model);
  }
#endif

  debug_layer_.Draw(scene_data_.view_projection);
}

void World::Update(float delta_time) {
  debug_layer_.Update(delta_time);
}

void World::OnClick(const base::Vector2f& pos) {
  Rayf ray = CreateRayFromScreen(pos.x, pos.y);
  debug_layer_.DrawVector(ray.origin, ray.direction, Vector3f{1.0f}, 1, true);
  selected_entity_ = SelectEntity(bvh_tree_, ray);
}

void World::UpdateRenderContext() {
  // Update the render context from the primary camera.
  for (auto [entity, _, camera, world_transform] :
       registry_.View<PrimaryCameraTag, CameraComponent,
                      WorldTransformComponent>()) {
    render_context_->proj.CreatePerspectiveProjection(
        camera.fov, (float)Engine::Get().GetScreenWidth(),
        (float)Engine::Get().GetScreenHeight(), camera.near_plane,
        camera.far_plane);
    world_transform.transform.InverseOrthogonal(render_context_->view);
    render_context_->view.Multiply(render_context_->proj,
                                   render_context_->view_proj);
    render_context_->camera_world_pos = world_transform.transform.Row(3);

    return;  // There can be only one primary camera.
  }
}

std::vector<World::DrawData> World::BuildDrawList(
    std::vector<SortItem> sorted_list) {
  DCHECK(!sorted_list.empty());

  instances_.clear();

  std::vector<DrawData> draw_list;

  uint32_t last_model_index = sorted_list[0].model_index;
  uint32_t instance_index = 0;
  uint32_t first_instance = 0;
  uint32_t instance_count = 0;

  for (auto& item : sorted_list) {
    if (item.model_index != last_model_index) {
      draw_list.emplace_back(last_model_index, first_instance, instance_count);
      last_model_index = item.model_index;
      first_instance = instance_index;
      instance_count = 0;
    }

    // This is not very cache-friendly (a few random accesses, but only on
    // visible items).
    instances_.emplace_back(world_transform_pool_->Get(item.entity).transform);
    ++instance_index;
    ++instance_count;
  }

  draw_list.emplace_back(last_model_index, first_instance, instance_count);
  return draw_list;
}

void World::UploadSceneData() {
  renderer_->UpdateBuffer(instances_ubo_, instances_.data(),
                          sizeof(InstanceData) * instances_.size());

  scene_data_.view_projection = render_context_->view_proj;
  scene_data_.cam_pos = render_context_->camera_world_pos;
  scene_data_.light_dir = {1, 1, 1};
  scene_data_.light_radiance = {1, 1, 1};
  scene_data_.white = render_context_->white;
  scene_data_.exposure = render_context_->exposure;
  renderer_->UpdateBuffer(scene_data_ubo_, &scene_data_, sizeof(scene_data_));

  renderer_->UpdateBuffer(lights_ubo_, &lights_, sizeof(lights_));
}

void World::SetParent(Entity entity, Entity new_parent) {
  DCHECK(entity != NULL_ENTITY);
  DCHECK(entity != new_parent);
  auto& scene_node = scene_node_pool_->Get(entity);
  uint32_t new_depth = NULL_INDEX;

  DetachFromParent(scene_node);

  // Link to new parent's sibling list (O(1)).
  scene_node.parent = new_parent;
  if (new_parent != NULL_ENTITY) {
    auto& new_parent_scene_node = scene_node_pool_->Get(new_parent);
    const Entity old_first_child = new_parent_scene_node.first_child;

    // Insert this entity at the front of the new parent's list.
    new_parent_scene_node.first_child = entity;
    scene_node.prev_sibling = NULL_ENTITY;
    scene_node.next_sibling = old_first_child;

    if (old_first_child != NULL_ENTITY) {
      // The old first child's prev must now point to us.
      scene_node_pool_->Get(old_first_child).prev_sibling = entity;
    }

    new_depth = new_parent_scene_node.depth + 1;
  } else {
    // This entity is now a root, clear its sibling pointers.
    scene_node.prev_sibling = NULL_ENTITY;
    scene_node.next_sibling = NULL_ENTITY;
    new_depth = 0;
  }

  OnHierarchyChanged(entity, new_depth);

  // Tag the entity dirty.
  dirty_tag_pool_->Add(entity, WorldTransformDirtyTag{});
}

void World::DetachFromParent(SceneNodeComponent& scene_node) {
  const Entity old_parent = scene_node.parent;

  // Unlink from old parent's sibling list (O(1)).
  if (old_parent != NULL_ENTITY) {
    auto& old_parent_scene_node = scene_node_pool_->Get(old_parent);
    const Entity prev = scene_node.prev_sibling;
    const Entity next = scene_node.next_sibling;

    // Unlink from the doubly-linked sibling list.
    if (prev != NULL_ENTITY) {
      scene_node_pool_->Get(prev).next_sibling = next;
    } else {
      // This was the first child, so update parent's pointer.
      old_parent_scene_node.first_child = next;
    }

    if (next != NULL_ENTITY) {
      // Our next sibling's prev must point to our prev.
      scene_node_pool_->Get(next).prev_sibling = prev;
    }
  }
}

void World::DestroyEntityAndChildren(Entity entity) {
  DCHECK(entity != NULL_ENTITY);

  DetachFromParent(scene_node_pool_->Get(entity));

  // Cascade delete all children.
  std::deque<Entity> stack;
  stack.push_back(entity);

  while (!stack.empty()) {
    Entity entity_to_destroy = stack.back();
    stack.pop_back();

    OnHierarchyChanged(entity_to_destroy, NULL_INDEX);

    // Iterate the sibling list to find all children.
    Entity child = scene_node_pool_->Get(entity_to_destroy).first_child;
    while (child != NULL_ENTITY) {
      stack.push_back(child);
      child = scene_node_pool_->Get(child).next_sibling;
    }

    // Finally, destroy the entity itself.
    registry_.DestroyEntity(entity_to_destroy);
  }
}

void World::OnHierarchyChanged(Entity entity, uint32_t new_depth) {
  auto& scene_node = scene_node_pool_->Get(entity);
  if (scene_node.depth == new_depth)
    return;

  // Remove from old bucket (fast swap-and-pop)
  if (scene_node.depth != NULL_INDEX &&
      scene_node.depth < depth_buckets_.size()) {
    auto& bucket = depth_buckets_[scene_node.depth];

    // Overwrite the entity we want to remove with the last entity
    Entity last_entity = bucket.back();
    bucket[scene_node.bucket_index] = last_entity;

    // Update the moved entity's component to point to its new home
    scene_node_pool_->Get(last_entity).bucket_index = scene_node.bucket_index;

    // Remove the (now duplicate) last element
    bucket.pop_back();
  }

  // Add to new bucket
  if (new_depth != NULL_INDEX) {
    // Ensure enough buckets exist
    if (depth_buckets_.size() <= new_depth)
      depth_buckets_.resize(new_depth + 1);

    auto& new_bucket = depth_buckets_[new_depth];

    // Add to the new bucket and update the component with its new depth and
    // index.
    new_bucket.push_back(entity);
    scene_node.depth = new_depth;
    scene_node.bucket_index = new_bucket.size() - 1;
  } else {
    scene_node.depth = NULL_INDEX;
    scene_node.bucket_index = NULL_INDEX;
  }
}

void World::UpdateWoldTransforms() {
  // Nothing to do here if there is no dirty node.
  if (dirty_tag_pool_->IsEmpty())
    return;

  // Iterate sequentially through depth levels (0 -> 1 -> 2...)
  for (int d = 0; d < depth_buckets_.size(); ++d) {
    // Iterate only the entities at this specific depth
    for (Entity entity : depth_buckets_[d]) {
      auto& scene_node = scene_node_pool_->Get(entity);
      auto& world_transform = world_transform_pool_->Get(entity);

      bool parent_was_dirty = false;

      // Check Parent's Dirty State (if we aren't root)
      // Because we process in depth order, we know the parent is already
      // up-to-date for this frame.
      if (scene_node.parent != NULL_ENTITY) {
        parent_was_dirty = dirty_tag_pool_->Has(scene_node.parent);
      }

      // Check self dirty state
      bool self_is_dirty = dirty_tag_pool_->Has(entity);

      // Update if necessary
      if (self_is_dirty || parent_was_dirty) {
        const auto& local_transform = local_transform_pool_->Get(entity);

        // Calculate new world matrix
        if (scene_node.parent == NULL_ENTITY) {
          // Root object: World = Local
          world_transform.transform = local_transform.transform;
        } else {
          // Child object: World = ParentWorld * Local
          const auto& parent_world_transform =
              world_transform_pool_->Get(scene_node.parent);
          parent_world_transform.transform.Multiply(local_transform.transform,
                                                    world_transform.transform);
        }

        // Propagate dirtiness downward
        if (!self_is_dirty)
          dirty_tag_pool_->Add(entity, WorldTransformDirtyTag{});
      }
    }
  }
}

void World::UpdateWorldBounds() {
  for (auto [entity, model, world_transform, world_bounds, _] :
       registry_.View<ModelComponent, WorldTransformComponent,
                      WorldBoundsComponent, WorldTransformDirtyTag>()) {
    world_bounds.obb = OBBf{world_transform.transform, model.extents};
  }
}

void World::BuildBVHTree(std::vector<BVHBuildItem> items) {
  bvh_tree_.clear();

  if (items.empty())
    return;

  size_t bvh_tree_size = 2 * items.size() - 1;
  if (bvh_tree_.size() <= bvh_tree_size)
    bvh_tree_.resize(bvh_tree_size);

  uint32_t last_node_index = 0;

  // Create stack for depth-first traversal and start the process with the root
  // node using all items.
  std::deque<std::tuple<uint32_t, std::span<BVHBuildItem>>> stack;
  stack.push_back(
      std::make_tuple(last_node_index, std::span{items.data(), items.size()}));

  while (!stack.empty()) {
    auto [node_index, node_items] = std::move(stack.back());
    stack.pop_back();

    // If only one object remains, this is a leaf.
    if (node_items.size() == 1) {
      bvh_tree_[node_index].entity = node_items[0].entity;
      continue;
    }

    // Calculate the combined AABB for all node_items in this branch.
    for (auto& item : node_items) {
      bvh_tree_[node_index].aabb.Expand(item.aabb);
    }

    // Find the longest axis of the combined AABB to split along.
    Vector3f extent =
        bvh_tree_[node_index].aabb.max - bvh_tree_[node_index].aabb.min;
    int axis = 0;
    if (extent.y > extent.x)
      axis = 1;
    if (extent.z > extent.y)
      axis = 2;

    // Sort node_items along the chosen axis based on their center point.
    if (axis == 0) {
      std::sort(node_items.begin(), node_items.end(),
                [](BVHBuildItem& a, BVHBuildItem& b) {
                  return a.center.x < b.center.x;
                });
    } else if (axis == 1) {
      std::sort(node_items.begin(), node_items.end(),
                [](BVHBuildItem& a, BVHBuildItem& b) {
                  return a.center.y < b.center.y;
                });
    } else {  // axis == 2
      std::sort(node_items.begin(), node_items.end(),
                [](BVHBuildItem& a, BVHBuildItem& b) {
                  return a.center.z < b.center.z;
                });
    }

    // Split the items into two halves
    size_t mid = node_items.size() / 2;
    std::span<BVHBuildItem> left_branch(node_items.begin(),
                                        node_items.begin() + mid);
    std::span<BVHBuildItem> right_branch(node_items.begin() + mid,
                                         node_items.end());

    // Create the child nodes.
    bvh_tree_[node_index].left = ++last_node_index;
    bvh_tree_[node_index].right = ++last_node_index;

    // Push the children onto the stack.
    stack.push_back({bvh_tree_[node_index].left, std::move(right_branch)});
    stack.push_back({bvh_tree_[node_index].right, std::move(left_branch)});
  }
}

std::vector<World::SortItem> World::FrustumCull(const Frustumf& frustum) {
  std::vector<SortItem> visible_items;
  if (bvh_tree_.empty())
    return visible_items;

  // Create stack for depth-first traversal and start the process with the root
  // of the BVH tree.
  std::deque<uint32_t> stack;
  stack.push_back(0);

  while (!stack.empty()) {
    auto node_index = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (bvh_tree_[node_index].IsLeaf()) {
      auto entity = bvh_tree_[node_index].entity;
      // This is not very cache-friendly (a few random accesses, but only on
      // visible leafs).
      if (frustum.Intersects(world_bounds_pool_->Get(entity).obb,
                             world_transform_pool_->Get(entity).transform))
        visible_items.emplace_back(entity,
                                   model_pool_->Get(entity).model_index);
      continue;
    } else if (!frustum.Intersects(bvh_tree_[node_index].aabb)) {
      continue;
    }

    // The internal node passed tests, check its children
    if (bvh_tree_[node_index].left)
      stack.push_back(bvh_tree_[node_index].left);
    if (bvh_tree_[node_index].right)
      stack.push_back(bvh_tree_[node_index].right);
  }

  return visible_items;
}

void World::DumpBVHTree(const std::vector<BVHNode>& nodes,
                        uint32_t node_index,
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
  if (nodes[node_index].IsLeaf()) {
    out << "[Leaf] model_index: "
        << model_pool_->Get(nodes[node_index].entity).model_index << " ";
    world_bounds_pool_->Get(nodes[node_index].entity).obb.GetBoundBox(aabb);
  } else {
    out << "[Internal] ";
    aabb = nodes[node_index].aabb;
  }

  // Print bounding box info
  Vector3f center = (aabb.max + aabb.min) * 0.5f;
  Vector3f extent = (aabb.max - aabb.min) * 0.5f;
  out << "Center: " << center.ToString() << " Extent: " << extent.ToString();
  DLOG(0) << out.str();

  // Prepare the prefix for the children
  std::string child_prefix = prefix + (is_last ? "    " : "│   ");

  // Recurse for children (if they exist)
  if (!nodes[node_index].IsLeaf()) {
    // The right child is always the "last" one for its parent
    DumpBVHTree(nodes, nodes[node_index].left, child_prefix, false);
    DumpBVHTree(nodes, nodes[node_index].right, child_prefix, true);
  }
}

void World::DrawBVHTree(const std::vector<BVHNode>& nodes,
                        uint32_t node_index) {
  if (nodes.empty())
    return;

  if (nodes[node_index].IsLeaf()) {
    Vector3f color{1, 1, 0};
    if (nodes[node_index].entity == selected_entity_)
      color = {0, 1, 1};
    debug_layer_.DrawObb(world_bounds_pool_->Get(nodes[node_index].entity).obb,
                         color);
  } else {
    debug_layer_.DrawAabb(nodes[node_index].aabb, {1, 0, 1});
  }

  // Recurse for children (if they exist)
  if (!nodes[node_index].IsLeaf()) {
    // The right child is always the "last" one for its parent
    DrawBVHTree(nodes, nodes[node_index].left);
    DrawBVHTree(nodes, nodes[node_index].right);
  }
}

Rayf World::CreateRayFromScreen(float screen_x, float screen_y) {
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
Entity World::SelectEntity(const std::vector<BVHNode>& nodes, const Rayf& ray) {
  Entity selected_entity = NULL_ENTITY;
  float closest_distance{std::numeric_limits<float>::max()};

  std::deque<uint32_t> stack;
  stack.push_back(0);

  while (!stack.empty()) {
    auto node_index = stack.back();
    stack.pop_back();

    // If the node is a leaf, it's representing a single object. Otherwise It's
    // an internal node with children.
    if (nodes[node_index].IsLeaf()) {
      float distance = world_bounds_pool_->Get(nodes[node_index].entity)
                           .obb.IntersectRay(ray);
      if (distance >= 0.0f && distance < closest_distance) {
        closest_distance = distance;
        selected_entity = nodes[node_index].entity;
      }
      continue;
    } else if (nodes[node_index].aabb.IntersectRay(ray) < 0) {
      continue;
    }

    // The internal node passed tests, check its children
    if (nodes[node_index].left)
      stack.push_back(nodes[node_index].left);
    if (nodes[node_index].right)
      stack.push_back(nodes[node_index].right);
  }

  return selected_entity;
}

}  // namespace eng
