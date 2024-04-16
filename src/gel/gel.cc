#include "gel/gel.h"

#include "base/log.h"
#include "engine/game_factory.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

GAME_FACTORIES{GAME_CLASS(Gel)};

Gel::Gel() = default;

Gel::~Gel() = default;

bool Gel::Initialize() {
  git_.RefreshCommitHistory();
  return true;
}

void Gel::Update(float delta_time) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Gel", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::Text("%d", (int)git_.GetCommitHistorySize());

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

      // Commit history is a large vertical list. Use clipper to only submit
      // items that are in view.
      ImGuiListClipper clipper;
      clipper.Begin(git_.GetCommitHistorySize());
      while (clipper.Step()) {
        std::vector<Git::CommitInfo> commits =
            git_.GetCommitRange(clipper.DisplayStart, clipper.DisplayEnd);
        for (int row = clipper.DisplayStart, i = 0; row < clipper.DisplayEnd;
             ++row, ++i) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text(commits[i].commit.c_str(), 0, row);
          ImGui::TableSetColumnIndex(1);
          ImGui::Text(commits[i].author.c_str(), 1, row);
          ImGui::TableSetColumnIndex(2);
          ImGui::Text(commits[i].author_date.c_str(), 2, row);
        }
      }
      ImGui::EndTable();
    }
  }
  ImGui::End();
}
