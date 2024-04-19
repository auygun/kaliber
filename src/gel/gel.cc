#include "gel/gel.h"

#include "base/log.h"
#include "engine/game_factory.h"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_internal.h"

using namespace base;
using namespace eng;

void MyStyle() {
  ImVec4* colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_Border] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowPadding = ImVec2(8.00f, 8.00f);
  style.FramePadding = ImVec2(5.00f, 2.00f);
  style.CellPadding = ImVec2(6.00f, 6.00f);
  style.ItemSpacing = ImVec2(6.00f, 6.00f);
  style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
  style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
  style.IndentSpacing = 25;
  style.ScrollbarSize = 15;
  style.GrabMinSize = 10;
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 1;
  style.TabBorderSize = 1;
  style.WindowRounding = 7;
  style.ChildRounding = 4;
  style.FrameRounding = 3;
  style.PopupRounding = 4;
  style.ScrollbarRounding = 9;
  style.GrabRounding = 3;
  style.LogSliderDeadzone = 4;
  style.TabRounding = 4;
}

GAME_FACTORIES{GAME_CLASS(Gel)};

Gel::Gel() = default;

Gel::~Gel() = default;

bool Gel::Initialize() {
  git_log_.Run();
  git_diff_.Run();
  MyStyle();
  return true;
}

void Gel::Update(float delta_time) {
  git_log_.Update();
  git_diff_.Update();

#if 1
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Gel", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoBackground |
                       ImGuiWindowFlags_NoSavedSettings)) {
    bool refresh_button = ImGui::Button("Refresh");
    ImGui::SameLine();
    bool kill_button = ImGui::Button("Kill");
    ImGui::SameLine();
    ImGui::Text("%d", (int)git_log_.GetCommitHistory().size());

    if (refresh_button)
      git_log_.Run();
    if (kill_button)
      git_log_.Kill();

    if (ImGui::BeginChild("child1", ImVec2(0, 300),
                          ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY)) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
      if (ImGui::BeginTable(
              "table_scrolly", 3,
              ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
        ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Three", ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();

        if (refresh_button)
          ImGui::SetScrollY(0);

        // Commit history is a large vertical list. Use clipper to only submit
        // items that are in view.
        auto& commit_history = git_log_.GetCommitHistory();
        ImGuiListClipper clipper;
        clipper.Begin(commit_history.size());
        while (clipper.Step()) {
          for (int row = clipper.DisplayStart; row < clipper.DisplayEnd;
               ++row) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            static int selected_row = -1;
            if (ImGui::Selectable(commit_history[row].commit.c_str(),
                                  selected_row == row,
                                  ImGuiSelectableFlags_SpanAllColumns))
              selected_row = row;
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(commit_history[row].author.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(commit_history[row].author_date.c_str());
          }
        }
        ImGui::EndTable();
      }
      ImGui::PopStyleVar(2);
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("child2", ImVec2(0, 0), ImGuiChildFlags_Border)) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGuiListClipper clipper;
      clipper.Begin(50);
      while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
          using namespace std::string_literals;
          static int selected_file = -1;
          if (ImGui::Selectable(git_diff_.GetFiles()[i].c_str(),
                                selected_file == i))
            selected_file = i;
        }
      }
      ImGui::PopStyleVar();
    }
    ImGui::EndChild();
  }
  ImGui::End();
#else
  ImGui::ShowDemoWindow();
#endif
}
