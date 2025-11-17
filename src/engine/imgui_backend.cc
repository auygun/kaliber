#include "engine/imgui_backend.h"

#include <memory>

#include "base/log.h"
#include "engine/asset/shader_source.h"
#include "engine/input_codes.h"
#include "engine/platform/asset_file.h"
#include "engine/platform/platform.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui.h"

using namespace base;

namespace eng {

namespace {
const char vertex_description[] = "p2f;t2f;c4b";

static ImGuiKey TranslateKey(Key platform_key) {
  switch (platform_key) {
    // Letters
    case Key::A:
      return ImGuiKey_A;
    case Key::B:
      return ImGuiKey_B;
    case Key::C:
      return ImGuiKey_C;
    case Key::D:
      return ImGuiKey_D;
    case Key::E:
      return ImGuiKey_E;
    case Key::F:
      return ImGuiKey_F;
    case Key::G:
      return ImGuiKey_G;
    case Key::H:
      return ImGuiKey_H;
    case Key::I:
      return ImGuiKey_I;
    case Key::J:
      return ImGuiKey_J;
    case Key::K:
      return ImGuiKey_K;
    case Key::L:
      return ImGuiKey_L;
    case Key::M:
      return ImGuiKey_M;
    case Key::N:
      return ImGuiKey_N;
    case Key::O:
      return ImGuiKey_O;
    case Key::P:
      return ImGuiKey_P;
    case Key::Q:
      return ImGuiKey_Q;
    case Key::R:
      return ImGuiKey_R;
    case Key::S:
      return ImGuiKey_S;
    case Key::T:
      return ImGuiKey_T;
    case Key::U:
      return ImGuiKey_U;
    case Key::V:
      return ImGuiKey_V;
    case Key::W:
      return ImGuiKey_W;
    case Key::X:
      return ImGuiKey_X;
    case Key::Y:
      return ImGuiKey_Y;
    case Key::Z:
      return ImGuiKey_Z;

    // Main keyboard numbers (top row)
    case Key::Num0:
      return ImGuiKey_0;
    case Key::Num1:
      return ImGuiKey_1;
    case Key::Num2:
      return ImGuiKey_2;
    case Key::Num3:
      return ImGuiKey_3;
    case Key::Num4:
      return ImGuiKey_4;
    case Key::Num5:
      return ImGuiKey_5;
    case Key::Num6:
      return ImGuiKey_6;
    case Key::Num7:
      return ImGuiKey_7;
    case Key::Num8:
      return ImGuiKey_8;
    case Key::Num9:
      return ImGuiKey_9;

    // Function keys
    case Key::F1:
      return ImGuiKey_F1;
    case Key::F2:
      return ImGuiKey_F2;
    case Key::F3:
      return ImGuiKey_F3;
    case Key::F4:
      return ImGuiKey_F4;
    case Key::F5:
      return ImGuiKey_F5;
    case Key::F6:
      return ImGuiKey_F6;
    case Key::F7:
      return ImGuiKey_F7;
    case Key::F8:
      return ImGuiKey_F8;
    case Key::F9:
      return ImGuiKey_F9;
    case Key::F10:
      return ImGuiKey_F10;
    case Key::F11:
      return ImGuiKey_F11;
    case Key::F12:
      return ImGuiKey_F12;

    // Control keys
    case Key::Escape:
      return ImGuiKey_Escape;
    case Key::Space:
      return ImGuiKey_Space;
    case Key::Enter:
      return ImGuiKey_Enter;
    case Key::Tab:
      return ImGuiKey_Tab;
    case Key::Backspace:
      return ImGuiKey_Backspace;
    case Key::Insert:
      return ImGuiKey_Insert;
    case Key::Delete:
      return ImGuiKey_Delete;

    // Arrow/Navigation keys
    case Key::Up:
      return ImGuiKey_UpArrow;
    case Key::Down:
      return ImGuiKey_DownArrow;
    case Key::Left:
      return ImGuiKey_LeftArrow;
    case Key::Right:
      return ImGuiKey_RightArrow;
    case Key::PageUp:
      return ImGuiKey_PageUp;
    case Key::PageDown:
      return ImGuiKey_PageDown;
    case Key::Home:
      return ImGuiKey_Home;
    case Key::End:
      return ImGuiKey_End;

    // Modifier keys
    case Key::ShiftLeft:
      return ImGuiKey_LeftShift;
    case Key::ShiftRight:
      return ImGuiKey_RightShift;
    case Key::ControlLeft:
      return ImGuiKey_LeftCtrl;
    case Key::ControlRight:
      return ImGuiKey_RightCtrl;
    case Key::AltLeft:
      return ImGuiKey_LeftAlt;
    case Key::AltRight:
      return ImGuiKey_RightAlt;
    // Note: ImGui doesn't typically distinguish between left/right 'Super'
    // (Windows/Cmd) keys. If you add a Super key, you might map both Left/Right
    // to ImGuiKey_LeftSuper.

    // Default case for unmapped keys
    case Key::Unknown:
    case Key::MaxKeys:
    default:
      return ImGuiKey_None;
  }
}

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
    float size_pixels = is_mobile ? 64 : 16;

    // Basic Latin, Latin-1 Supplement, Latin Extended-A, Latin Extended-B
    static const ImWchar full_ranges[] = {
        0x0020,
        0x024F,
        0,  // Null terminator
    };

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        buffer.get(), (int)buffer_size, size_pixels, &font_cfg, full_ranges);
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

std::pair<bool, bool> ImguiBackend::ProcessInput(Platform* platform) {
  // TODO: Use PlayerInput component
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent(platform->GetMouseX(), platform->GetMouseY());
  io.AddMouseButtonEvent(ImGuiMouseButton_Left,
                         platform->IsMouseButtonDown(MouseButton::Left));
  io.AddMouseButtonEvent(ImGuiMouseButton_Right,
                         platform->IsMouseButtonDown(MouseButton::Right));
  io.AddMouseButtonEvent(ImGuiMouseButton_Middle,
                         platform->IsMouseButtonDown(MouseButton::Middle));
  io.AddMouseWheelEvent(0.0f, platform->GetMouseScrollDelta() * 0.2f);

  for (size_t i = 0; i < static_cast<size_t>(Key::MaxKeys); ++i) {
    auto imgui_key = TranslateKey(static_cast<Key>(i));
    bool is_down = platform->IsKeyDown(static_cast<Key>(i));
    io.AddKeyEvent(imgui_key, is_down);
  }

  // Character input
  const auto& chars = platform->GetInputCharacters();
  for (unsigned int c : chars) {
    io.AddInputCharacter(c);
  }

  return std::make_pair(io.WantCaptureMouse, io.WantCaptureKeyboard);
}

void ImguiBackend::NewFrame(float delta_time) {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)renderer_->GetScreenWidth(),
                          (float)renderer_->GetScreenHeight());
  io.DeltaTime = delta_time;
  ImGui::NewFrame();
  needs_update_ = true;
}

// void ImguiBackend::EndFrame() {
//   ImGui::EndFrame();
// }

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
