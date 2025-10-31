#include "engine/imgui_backend.h"

#include "base/log.h"
#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/input_event.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui.h"

using namespace base;

namespace eng {

namespace {
const char vertex_description[] = "p2f;t2f;c4b";
}  // namespace

ImguiBackend::ImguiBackend() = default;

ImguiBackend::~ImguiBackend() = default;

void ImguiBackend::Initialize(bool is_mobile, std::string root_path) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  if (!ParseVertexDescription(vertex_description, vertex_description_))
    LOG(0) << "Failed to parse vertex description.";

  size_t buffer_size = 0;
  auto buffer = AssetFile::ReadWholeFile("engine/RobotoMono-Regular.ttf",
                                         root_path.c_str(), &buffer_size);
  if (buffer) {
    ImFontConfig font_cfg = ImFontConfig();
    font_cfg.FontDataOwnedByAtlas = false;
    float size_pixels = is_mobile ? 64 : 32;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(buffer.get(), (int)buffer_size,
                                               size_pixels, &font_cfg);
    ImGui::GetIO().Fonts->Build();
  } else {
    LOG(0) << "Failed to read font file.";
  }

  // Arbitrary scale-up for mobile devices.
  // TODO: Put some effort into DPI awareness.
  if (is_mobile)
    ImGui::GetStyle().ScaleAllSizes(2.0f);
}

void ImguiBackend::Shutdown() {
  ImGui::DestroyContext();

  renderer_->DestroyDescriptorSet(texture_dset_);
  renderer_->DestroyDescriptorSet(scene_dset_);
  renderer_->DestroyBuffer(scene_data_ubo_);

  for (auto geometry : geometries_)
    renderer_->DestroyGeometry(geometry);
  geometries_.clear();

  renderer_->DestroyTexture(font_atlas_);
  renderer_->DestroyShader(shader_);
}

void ImguiBackend::CreateRenderResources(Renderer* renderer) {
  renderer_ = renderer;

  // Create the shader.
  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/imgui.glsl")) {
    shader_ = renderer_->CreateShader(std::move(source), vertex_description_,
                                      kPrimitive_Triangles, false, false,
                                      CullMode::kNone);
  } else {
    LOG(0) << "Could not create imgui shader.";
  }

  // Create a texture for the font atlas.
  unsigned char* pixels;
  int width, height;
  ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  LOG(0) << "Font atlas size: " << width << ", " << height;
  font_atlas_ = renderer_->CreateTexture();
  renderer_->UpdateTexture(font_atlas_, width, height, 1, 0,
                           ImageFormat::kRGBA32, width * height * 4, pixels);

  texture_dset_ =
      renderer_->CreateDescriptorSet(shader_, 2, {{font_atlas_}}, {});
  ImGui::GetIO().Fonts->SetTexID((ImTextureID)(intptr_t)texture_dset_);

  scene_data_ubo_ = renderer_->CreateBuffer(shader_, 1, 0, sizeof(SceneData));
  scene_dset_ =
      renderer_->CreateDescriptorSet(shader_, 1, {}, {scene_data_ubo_});
}

std::unique_ptr<InputEvent> ImguiBackend::OnInputEvent(
    std::unique_ptr<InputEvent> event) {
  ImGuiIO& io = ImGui::GetIO();
  switch (event->GetType()) {
    case InputEvent::kDragStart:
      io.AddMousePosEvent(event->GetVector().x, event->GetVector().y);
      io.AddMouseButtonEvent(0, true);
      break;
    case InputEvent::kDragEnd:
      io.AddMousePosEvent(event->GetVector().x, event->GetVector().y);
      io.AddMouseButtonEvent(0, false);
      break;
    case InputEvent::kDrag:
      io.AddMousePosEvent(event->GetVector().x, event->GetVector().y);
      break;
    default:
      break;
  }
  // TODO: Keyboard input

  if (io.WantCaptureMouse)
    event.reset();
  return event;
}

void ImguiBackend::NewFrame(float delta_time) {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)renderer_->GetScreenWidth(),
                          (float)renderer_->GetScreenHeight());
  io.DeltaTime = delta_time;
  ImGui::NewFrame();
  needs_update_ = true;
}

void ImguiBackend::EndFrame() {
  ImGui::EndFrame();
}

void ImguiBackend::UpdateGeometries() {
  // Create a geometry for each draw list and upload the vertex data.
  ImDrawData* draw_data = ImGui::GetDrawData();
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    if ((int)geometries_.size() <= n)
      geometries_.emplace_back((uint64_t)-1);
    if (geometries_[n] == (uint64_t)-1)
      geometries_[n] = renderer_->CreateGeometry(
          kPrimitive_Triangles, vertex_description_, kDataType_UShort);
    renderer_->UpdateGeometry(
        geometries_[n], cmd_list->VtxBuffer.Size, cmd_list->VtxBuffer.Data,
        cmd_list->IdxBuffer.Size, cmd_list->IdxBuffer.Data);
  }
}

void ImguiBackend::Draw() {
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();
  if (!draw_data || draw_data->CmdListsCount <= 0)
    return;

  if (needs_update_) {
    UpdateGeometries();
    needs_update_ = false;
  }

  scene_data_.projection.CreateOrthographicProjection(
      draw_data->DisplayPos.x,
      draw_data->DisplayPos.x + draw_data->DisplaySize.x,
      draw_data->DisplayPos.y + draw_data->DisplaySize.y,
      draw_data->DisplayPos.y);
  renderer_->UpdateBuffer(scene_data_ubo_, &scene_data_, sizeof(scene_data_));

  renderer_->SetViewport(0, 0, draw_data->DisplaySize.x,
                         draw_data->DisplaySize.y);
  renderer_->ActivateShader(shader_);
  renderer_->ActivateDescriptorSet(scene_dset_);

  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    renderer_->ActivateGeometry(geometries_[n]);

    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->ClipRect.z <= pcmd->ClipRect.x ||
          pcmd->ClipRect.w <= pcmd->ClipRect.y)
        continue;
      renderer_->ActivateDescriptorSet((uint32_t)(intptr_t)pcmd->GetTexID());
      renderer_->SetScissor(int(pcmd->ClipRect.x), int(pcmd->ClipRect.y),
                            int(pcmd->ClipRect.z - pcmd->ClipRect.x),
                            int(pcmd->ClipRect.w - pcmd->ClipRect.y));
      renderer_->Draw(pcmd->ElemCount, pcmd->IdxOffset);
    }
  }
  renderer_->ResetScissor();
  renderer_->ResetViewport();
}

}  // namespace eng
