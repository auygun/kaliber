#include "gel/gel.h"

#include "base/log.h"
#include "engine/game_factory.h"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_internal.h"

using namespace base;
using namespace eng;

GAME_FACTORIES{GAME_CLASS(Gel)};

Gel::Gel() = default;

Gel::~Gel() = default;

bool Gel::Initialize() {
  git_log_.Run();
  return true;
}

void Gel::Update(float delta_time) {
  git_log_.Update();

#if 1
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Gel", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
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

    static ImGuiTableFlags flags =
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable;

    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
    if (ImGui::BeginTable("table_scrolly", 3, flags, outer_size)) {
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
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text(commit_history[row].commit.c_str(), 0, row);
          ImGui::TableSetColumnIndex(1);
          ImGui::Text(commit_history[row].author.c_str(), 1, row);
          ImGui::TableSetColumnIndex(2);
          ImGui::Text(commit_history[row].author_date.c_str(), 2, row);
        }
      }
      ImGui::EndTable();
    }
  }
  ImGui::End();
#else
  ImGui::ShowDemoWindow();
#endif
}
