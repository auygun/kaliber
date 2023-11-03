#include "engine/imgui_backend.h"

#include "base/log.h"
#include "engine/asset/shader_source.h"
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
  for (auto& id : geometries_)
    renderer_->DestroyGeometry(id);
  geometries_.clear();
  renderer_->DestroyTexture(font_atlas_);
  renderer_->DestroyShader(shader_);
}

void ImguiBackend::CreateRenderResources(Renderer* renderer) {
  renderer_ = renderer;
  geometries_.clear();

  // Avoid flickering by using the geometries form the last frame if available.
  if (ImGui::GetCurrentContext() && ImGui::GetDrawData())
    Render();

  // Create the shader.
  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/imgui.glsl")) {
    shader_ = renderer_->CreateShader(std::move(source), vertex_description_,
                                      kPrimitive_Triangles, false);
  } else {
    LOG(0) << "Could not create imgui shader.";
  }

  // Create a texture for the font atlas.
  unsigned char* pixels;
  int width, height;
  ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  LOG(0) << "Font atlas size: " << width << ", " << height;
  font_atlas_ = renderer_->CreateTexture();
  renderer_->UpdateTexture(font_atlas_, width, height, ImageFormat::kRGBA32,
                           width * height * 4, pixels);
  ImGui::GetIO().Fonts->SetTexID((ImTextureID)(intptr_t)font_atlas_);
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
}

void ImguiBackend::Render() {
  ImGui::Render();
  // Create a geometry for each draw list and upload the vertex data.
  ImDrawData* draw_data = ImGui::GetDrawData();
  if ((int)geometries_.size() < draw_data->CmdListsCount)
    geometries_.resize(draw_data->CmdListsCount, 0);
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    if (geometries_[n] == 0)
      geometries_[n] = renderer_->CreateGeometry(
          kPrimitive_Triangles, vertex_description_, kDataType_UShort);
    renderer_->UpdateGeometry(
        geometries_[n], cmd_list->VtxBuffer.Size, cmd_list->VtxBuffer.Data,
        cmd_list->IdxBuffer.Size, cmd_list->IdxBuffer.Data);
  }
}

void ImguiBackend::Draw() {
  ImDrawData* draw_data = ImGui::GetDrawData();
  if (!draw_data || draw_data->CmdListsCount <= 0)
    return;

  renderer_->SetViewport(0, 0, draw_data->DisplaySize.x,
                         draw_data->DisplaySize.y);

  base::Matrix4f proj;
  proj.CreateOrthoProjection(draw_data->DisplayPos.x,
                             draw_data->DisplayPos.x + draw_data->DisplaySize.x,
                             draw_data->DisplayPos.y + draw_data->DisplaySize.y,
                             draw_data->DisplayPos.y);
  renderer_->ActivateShader(shader_);
  renderer_->SetUniform(shader_, "projection", proj);
  renderer_->UploadUniforms(shader_);

  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->ClipRect.z <= pcmd->ClipRect.x ||
          pcmd->ClipRect.w <= pcmd->ClipRect.y)
        continue;
      auto texture_id = reinterpret_cast<uint64_t>(pcmd->GetTexID());
      renderer_->ActivateTexture(texture_id, 0);
      renderer_->SetScissor(int(pcmd->ClipRect.x), int(pcmd->ClipRect.y),
                            int(pcmd->ClipRect.z - pcmd->ClipRect.x),
                            int(pcmd->ClipRect.w - pcmd->ClipRect.y));
      renderer_->Draw(geometries_[n], pcmd->ElemCount, pcmd->IdxOffset);
    }
  }
  renderer_->ResetScissor();
  renderer_->ResetViewport();
}

}  // namespace eng
