#include "engine/imgui_backend.h"

#include "base/log.h"
#include "engine/asset/image.h"
#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/input_event.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"
#include "third_party/imgui/imgui.h"

using namespace base;

namespace eng {

namespace {
const char vertex_description[] = "p2f;t2f;c4b";
}  // namespace

ImguiBackend::ImguiBackend() : shader_{std::make_unique<Shader>(nullptr)} {}

ImguiBackend::~ImguiBackend() = default;

void ImguiBackend::Initialize() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  if (!ParseVertexDescription(vertex_description, vertex_description_))
    LOG(0) << "Failed to parse vertex description.";

  size_t buffer_size = 0;
  auto buffer = AssetFile::ReadWholeFile("engine/RobotoMono-Regular.ttf",
                                         Engine::Get().GetRootPath().c_str(),
                                         &buffer_size);
  if (buffer) {
    ImFontConfig font_cfg = ImFontConfig();
    font_cfg.FontDataOwnedByAtlas = false;
    float size_pixels = Engine::Get().IsMobile() ? 64 : 32;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(buffer.get(), (int)buffer_size,
                                               size_pixels, &font_cfg);
    ImGui::GetIO().Fonts->Build();
  } else {
    LOG(0) << "Failed to read font file.";
  }

  // Arbitrary scale-up for mobile devices.
  // TODO: Put some effort into DPI awareness.
  if (Engine::Get().IsMobile())
    ImGui::GetStyle().ScaleAllSizes(2.0f);

  Engine::Get().SetImageSource(
      "imgui_atlas",
      []() -> std::unique_ptr<Image> {
        unsigned char* pixels;
        int width, height;
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        LOG(0) << "Font atlas size: " << width << ", " << height;
        auto image = std::make_unique<Image>();
        image->Create(width, height);
        memcpy(image->GetBuffer(), pixels, width * height * 4);
        return image;
      },
      true);
  Engine::Get().RefreshImage("imgui_atlas");
  ImGui::GetIO().Fonts->SetTexID(
      (ImTextureID)(intptr_t)Engine::Get().AcquireTexture("imgui_atlas"));
}

void ImguiBackend::Shutdown() {
  for (auto& id : geometries_)
    renderer_->DestroyGeometry(id);
  geometries_.clear();
  ImGui::DestroyContext();
  shader_.reset();
}

void ImguiBackend::CreateRenderResources(Renderer* renderer) {
  renderer_ = renderer;
  shader_->SetRenderer(renderer);

  geometries_.clear();
  if (ImGui::GetCurrentContext() && ImGui::GetDrawData())
    Render();

  auto source = std::make_unique<ShaderSource>();
  if (source->Load("engine/imgui.glsl")) {
    VertexDescription vd;
    if (!ParseVertexDescription(vertex_description, vd)) {
      DLOG(0) << "Failed to parse vertex description.";
    } else {
      shader_->Create(std::move(source), vd, kPrimitive_Triangles, false);
    }
  } else {
    LOG(0) << "Could not create imgui shader.";
  }
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
  shader_->Activate();
  shader_->SetUniform("projection", proj);
  shader_->UploadUniforms();

  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->ClipRect.z <= pcmd->ClipRect.x ||
          pcmd->ClipRect.w <= pcmd->ClipRect.y)
        continue;
      reinterpret_cast<Texture*>(pcmd->GetTexID())->Activate(0);
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
