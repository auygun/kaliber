#include "engine/engine.h"

#include "base/log.h"
#include "base/task_runner.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/asset/sound.h"
#include "engine/fly_camera.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "engine/input_system.h"
#include "engine/orbit_camera.h"
#include "engine/platform/platform.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui/imgui.h"
#include "third_party/texture_compressor/texture_compressor.h"

using namespace base;

namespace eng {

extern void KaliberMain(Platform* platform) {
  TaskRunner::CreateThreadLocalTaskRunner();
  Engine(platform).Run();
}

Engine* Engine::singleton = nullptr;

Engine::Engine(Platform* platform) : platform_(platform) {
  DCHECK(!singleton);
  singleton = this;

  platform_->SetObserver(this);
}

Engine::~Engine() noexcept {
  LOG(0) << "Shutting down engine.";

  thread_pool_.CancelTasks();
  singleton = nullptr;
}

Engine& Engine::Get() {
  return *singleton;
}

void Engine::Run() {
  Initialize();

  timer_ = DeltaTimer();
  float accumulator = 0.0f;

  // Maximum time we allow the simulation to fall behind real-time.
  constexpr float max_accumulator = 0.25f;  // 250ms is a common safe value.

  for (;;) {
    platform_->Update();
    if (platform_->should_exit())
      return;

    if (!renderer_->IsInitialzed())
      continue;

    // Capture frame time once.
    float frame_delta = timer_.Delta();
    accumulator += frame_delta;

    // Safety clamp to prevent "spiral of death" on lag spikes.
    if (accumulator > max_accumulator)
      accumulator = max_accumulator;

    // Subdivide the frame time using fixed time steps.
    while (accumulator >= time_step_) {
      FixedUpdate(time_step_);
      accumulator -= time_step_;
    };

    // Variable update
    Update(frame_delta);

    // Calculate frame fraction from remainder of the frame time.
    float frame_frac = accumulator / time_step_;
    Draw(frame_frac);
  }
}

void Engine::Initialize() {
  LOG(0) << "Initializing engine.";

  thread_pool_.Initialize();

  input_system_.Init(world_.GetRegistry());

  platform_->CreateMainWindow();

  // After the window exists, so the backend picks up the display scale that
  // CreateMainWindow() detected.
  imgui_backend_.Initialize(
      platform_, GetRootPath() + "assets/engine/RobotoMono-Regular.ttf");

  CreateRendererInternal(RendererType::kVulkan);

  float aspect_ratio =
      (float)GetFramebufferHeight() / (float)GetFramebufferWidth();
  LOG(0) << "aspect_ratio: " << aspect_ratio;
  screen_size_ = {1.0f, aspect_ratio * 1.0f};

  world_.Create(renderer_.get());

  input_system_.Init(world_.GetRegistry());

  systems_.push_back(std::make_unique<FlyCamera>());
  // systems_.push_back(std::make_unique<OrbitCamera>());
  systems_.back()->Init(world_);

  game_ = GameFactoryBase::CreateGame("");
  CHECK(game_) << "No game found to run.";
  CHECK(game_->Initialize(world_)) << "Failed to initialize the game.";

  imgui_backend_.CreateRenderResources(renderer_.get());

  render_graph_.Initialize(renderer_.get());

  auto cam_entity = world_.CreateSceneNode("cam");
  auto& registry = world_.GetRegistry();
  registry.AddComponent(cam_entity, FlyCameraComponent{.speed = 4.0f});
  // registry.AddComponent(cam_entity, OrbitCameraComponent{.speed = 200.0f});
  registry.AddComponent(cam_entity, PrimaryCameraTag{});
  registry.AddComponent(
      cam_entity,
      CameraComponent{.fov = 45.0f, .near_plane = 1.0f, .far_plane = 1000.0f});
}

void Engine::FixedUpdate(float delta_time) {
  seconds_accumulated_ += delta_time;
  ++tick_;

  game_->FixedUpdate(world_);
}

void Engine::Update(float delta_time) {
  imgui_backend_.NewFrame(delta_time);
  auto [mouse_captured, keyboard_captured] =
      imgui_backend_.ProcessInput(platform_);

  input_system_.Update(mouse_captured, keyboard_captured);

  // Scene graph update for objects moved by physics
  world_.UpdateSceneGraph();

  TaskRunner::GetThreadLocalTaskRunner()->RunTasks<Consumer::Single>();

  for (auto& system : systems_)
    system->Update(world_, delta_time);

  game_->Update(world_, delta_time);

  world_.Update(delta_time);

  // Scene graph update (Post-Logic finalize)
  world_.UpdateSceneGraph();

  fps_seconds_ += delta_time;
  if (fps_seconds_ >= 1) {
    fps_ = renderer_->GetAndResetFPS();
    fps_seconds_ = 0;
  }

  if (stats_visible_)
    ShowStats();

  DrawSceneGraphUI();
}

void Engine::Draw(float frame_frac) {
  renderer_->PrepareForDrawing();

  render_graph_.Reset();
  render_graph_.AddPass(
      "scene", "scene_layer",
      [this, frame_frac](RenderGraphContext& ctx) {
        world_.Render(frame_frac);
      },
      true);
  render_graph_.AddPass("ui", "ui_layer", [this](RenderGraphContext& ctx) {
    imgui_backend_.Draw();
  });
  render_graph_.Execute(renderer_.get());
  renderer_->Present();
}

void Engine::CreateRenderer(RendererType type) {
  // Create a new renderer next cycle.
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::CreateRendererInternal, this, type));
  TaskRunner::TaskRunner::GetThreadLocalTaskRunner()->PostTask(
      HERE, std::bind(&Engine::ContextLost, this));
}

RendererType Engine::GetRendererType() {
  if (renderer_)
    return renderer_->GetRendererType();
  return RendererType::kUnknown;
}

void Engine::Exit() {
  platform_->Exit();
}

Vector2f Engine::ToViewportScale(const Vector2f& vec) {
  return GetViewportSize() * vec /
         Vector2f((float)GetFramebufferWidth(), (float)GetFramebufferHeight());
}

Vector2f Engine::ToViewportPosition(const Vector2f& vec) {
  return ToViewportScale(vec) - GetViewportSize() / 2.0f;
}

void Engine::Vibrate(int duration) {
  if (vibration_enabled_)
    platform_->Vibrate(duration);
}

void Engine::ShowInterstitialAd() {
  platform_->ShowInterstitialAd();
}

void Engine::ShareFile(const std::string& file_name) {
  platform_->ShareFile(file_name);
}

void Engine::SetKeepScreenOn(bool keep_screen_on) {
  platform_->SetKeepScreenOn(keep_screen_on);
}

void Engine::SetEnableAudio(bool enable) {
  audio_mixer_.SetEnableAudio(enable);
}

TextureCompressor* Engine::GetTextureCompressor(bool opacity) {
  return opacity ? tex_comp_alpha_.get() : tex_comp_opaque_.get();
}

int Engine::GetFramebufferWidth() const {
  return renderer_->GetFramebufferWidth();
}

int Engine::GetFramebufferHeight() const {
  return renderer_->GetFramebufferHeight();
}

const std::string& Engine::GetRootPath() const {
  return platform_->GetRootPath();
}

const std::string& Engine::GetDataPath() const {
  return platform_->GetDataPath();
}

const std::string& Engine::GetSharedDataPath() const {
  return platform_->GetSharedDataPath();
}

size_t Engine::GetAudioHardwareSampleRate() {
  return audio_mixer_.GetHardwareSampleRate();
}

bool Engine::IsMobile() const {
  return platform_->mobile_device();
}

void Engine::OnWindowCreated() {
  if (renderer_)
    renderer_->Initialize(platform_);
}

void Engine::OnWindowDestroyed() {
  renderer_->Shutdown();
}

void Engine::OnFramebufferResized(int width, int height) {
  if (renderer_ && (width != renderer_->GetFramebufferWidth() ||
                    height != renderer_->GetFramebufferHeight())) {
    renderer_->OnFramebufferResized(width, height);
    float aspect_ratio =
        (float)GetFramebufferHeight() / (float)GetFramebufferWidth();
    LOG(0) << "aspect_ratio: " << aspect_ratio;
    screen_size_ = {1.0f, aspect_ratio * 1.0f};
    game_->OnFramebufferResized(width, height);
  }
}

void Engine::LostFocus() {
  audio_mixer_.Suspend();

  if (game_)
    game_->LostFocus();
}

void Engine::GainedFocus() {
  timer_ = DeltaTimer();
  audio_mixer_.Resume();

  if (game_)
    game_->GainedFocus(platform_->gained_focus_from_interstitial_ad());
}

void Engine::CreateRendererInternal(RendererType type) {
  if (renderer_ && renderer_->GetRendererType() == type)
    return;

  renderer_ = Renderer::Create(type, std::bind(&Engine::ContextLost, this));
  bool result = renderer_->Initialize(platform_);
  CHECK(result) << "Failed to initialize " << renderer_->GetDebugName()
                << " renderer.";

  CreateTextureCompressors();
}

void Engine::CreateTextureCompressors() {
  tex_comp_alpha_.reset();
  tex_comp_opaque_.reset();

  if (renderer_->SupportsDXT5()) {
    tex_comp_alpha_ = TextureCompressor::Create(TextureCompressor::kFormatDXT5);
  } else if (renderer_->SupportsATC()) {
    tex_comp_alpha_ =
        TextureCompressor::Create(TextureCompressor::kFormatATCIA);
  }

  if (renderer_->SupportsDXT1()) {
    tex_comp_opaque_ =
        TextureCompressor::Create(TextureCompressor::kFormatDXT1);
  } else if (renderer_->SupportsATC()) {
    tex_comp_opaque_ = TextureCompressor::Create(TextureCompressor::kFormatATC);
  } else if (renderer_->SupportsETC1()) {
    tex_comp_opaque_ =
        TextureCompressor::Create(TextureCompressor::kFormatETC1);
  }
}

void Engine::ContextLost() {
  render_graph_.ContextLost();
  // The imgui backend holds shader, geometry and texture handles that died
  // with the old context. Rebuild them against the current renderer.
  if (renderer_)
    imgui_backend_.CreateRenderResources(renderer_.get());
  if (game_)
    game_->ContextLost();
}

void Engine::ShowStats() {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Stats", nullptr, window_flags);
  ImGui::Text("%s", renderer_->GetDebugName());
  ImGui::Text("%d fps", fps_);
  ImGui::End();
}

// Iteratively draws a single root node and all its descendants.
void Engine::DrawSceneNodeIterative(Entity root_entity) {
  Registry& registry = world_.GetRegistry();

  // The stack for our depth-first traversal.
  // We need to store the entity and the depth for ImGui::TreePop.
  struct NodeDepth {
    Entity entity;
    int depth;  // Number of TreePop() calls needed when we ascend from here
  };

  std::deque<NodeDepth> stack;
  stack.push_back({root_entity, 0});

  while (!stack.empty()) {
    NodeDepth current = stack.front();
    stack.pop_front();

    if (!registry.HasComponent<SceneNodeComponent>(current.entity))
      continue;

    // If depth is -1, it's a marker to call TreePop
    if (current.depth == -1) {
      ImGui::TreePop();
      continue;
    }

    auto& node = registry.GetComponent<SceneNodeComponent>(current.entity);

    // Set tree node flags
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node.first_child == NULL_ENTITY) {
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (current.entity == selected_entity_) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Draw the tree node ---
    bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)current.entity, flags,
                                       "%s", node.name);

    // Handle selection
    if (ImGui::IsItemClicked()) {
      selected_entity_ = current.entity;
    }

    // Handle drag source (this node is being dragged)
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("SCENE_NODE_ENTITY", &current.entity,
                                sizeof(Entity));
      ImGui::Text("%s", node.name);
      ImGui::EndDragDropSource();
    }

    // Handle drag target (another node is dropped on this one)
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload =
              ImGui::AcceptDragDropPayload("SCENE_NODE_ENTITY")) {
        Entity dragged_entity = *static_cast<Entity*>(payload->Data);

        // Check for invalid parenting (self, or no-op)
        if (dragged_entity != current.entity && dragged_entity != node.parent) {
          // Defer reparenting until after the loop
          dragged_entity_ = dragged_entity;
          new_parent_entity_ = current.entity;
        }
      }
      ImGui::EndDragDropTarget();
    }

    // Handle traversal
    if (node_open && !(flags & ImGuiTreeNodeFlags_Leaf)) {
      // Add a pop marker to the stack, to be processed after all children
      stack.push_front({current.entity, -1});

      // Add children to the front of the stack (deque) in reverse order to
      // process them in the correct (forward) order.
      Entity child = node.first_child;
      std::vector<Entity> children;
      while (child != NULL_ENTITY) {
        if (!registry.HasComponent<SceneNodeComponent>(child))
          break;
        children.push_back(child);
        child = registry.GetComponent<SceneNodeComponent>(child).next_sibling;
      }

      // Push children onto the stack (front of deque) in reverse
      for (auto it = children.rbegin(); it != children.rend(); ++it) {
        stack.push_front({*it, current.depth + 1});
      }
    }
  }
}

// Draws the inspector panel for the currently selected node.
void Engine::DrawNodeInspector() {
  Registry& registry = world_.GetRegistry();

  // 1. Handle "No Selection"
  if (selected_entity_ == NULL_ENTITY) {
    ImGui::Text("Select a node to inspect.");
    return;
  }

  // 2. Handle "Invalid Selection" (e.g., entity was destroyed)
  if (!registry.HasComponent<SceneNodeComponent>(selected_entity_)) {
    ImGui::Text("Selected node is no longer valid.");
    // Clear the selection so we don't check a bad pointer next frame
    selected_entity_ = NULL_ENTITY;
    return;
  }

  // 3. Draw Inspector UI
  auto& node = registry.GetComponent<SceneNodeComponent>(selected_entity_);

  // Entity ID (read-only)
  ImGui::Text("Entity ID: %u", selected_entity_);

  // Name (Editable)
  // We must copy the fixed-size char array to a temporary buffer for ImGui
  char name_buffer[8];
  strncpy(name_buffer, node.name, 8);

  if (ImGui::InputText("Name", name_buffer, 8)) {
    // Copy the edited name back into the component
    strncpy(node.name, name_buffer, 8);
  }

  // Parent ID (read-only)
  ImGui::Text("Parent ID: %u", node.parent);

  // Child Count (Calculated)
  int child_count = 0;
  Entity child = node.first_child;
  while (child != NULL_ENTITY) {
    if (!registry.HasComponent<SceneNodeComponent>(child))
      break;
    child_count++;
    child = registry.GetComponent<SceneNodeComponent>(child).next_sibling;
  }
  ImGui::Text("Children: %d", child_count);
}

// Creates the scene graph ImGui window. Calls the iterative drawing function
// for all root nodes. Handles deferred reparenting after the UI is drawn.
void Engine::DrawSceneGraphUI() {
  Registry& registry = world_.GetRegistry();

  dragged_entity_ = NULL_ENTITY;
  new_parent_entity_ = NULL_ENTITY;

  ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Scene Hierarchy")) {
    static float top_pane_height = 250.0f;  // Height of the tree view
    float splitter_thickness = 8.0f;        // A thicker bar for easier grabbing

    // Top Pane (Tree View)
    ImGui::BeginChild("SceneGraphTree", ImVec2(0, top_pane_height), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    {
      for (auto [entity, node] : registry.View<SceneNodeComponent>()) {
        if (node.parent == NULL_ENTITY) {
          DrawSceneNodeIterative(entity);
        }
      }
    }
    ImGui::EndChild();

    // Horizontal Splitter
    ImGui::InvisibleButton("##h_splitter", ImVec2(-1, splitter_thickness));

    if (ImGui::IsItemHovered()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (ImGui::IsItemActive()) {
      // Adjust top pane height based on mouse delta
      top_pane_height += ImGui::GetIO().MouseDelta.y;

      // Add constraints to prevent panes from collapsing
      float min_height = 40.0f;
      float max_height =
          ImGui::GetContentRegionAvail().y - splitter_thickness - min_height;
      if (top_pane_height < min_height)
        top_pane_height = min_height;
      if (top_pane_height > max_height)
        top_pane_height = max_height;
    }

    // Draw a visual line for the splitter
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 min_rect = ImGui::GetItemRectMin();
    ImVec2 max_rect = ImGui::GetItemRectMax();
    draw_list->AddRectFilled(min_rect, max_rect,
                             ImGui::GetColorU32(ImGuiCol_SeparatorHovered));

    // Bottom Pane (Inspector)
    ImGui::BeginChild("NodeInspector", ImVec2(0, 0), true);
    {
      DrawNodeInspector();
    }
    ImGui::EndChild();
  }
  ImGui::End();

  // Handle deferred reparenting
  if (dragged_entity_ != NULL_ENTITY) {
    world_.SetParent(dragged_entity_, new_parent_entity_);
  }
}

}  // namespace eng
