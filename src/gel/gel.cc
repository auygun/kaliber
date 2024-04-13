#include "gel/gel.h"

#include "base/closure.h"
#include "engine/engine.h"
#include "engine/game_factory.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

GAME_FACTORIES{GAME_CLASS(Gel)};

Gel::Gel() = default;

Gel::~Gel() {
  proc_runner_.Shutdown();
}

bool Gel::Initialize() {
  proc_runner_.Initialize(
      std::bind(&Gel::OnGitOutput, this, std::placeholders::_1,
                std::placeholders::_2),
      std::bind(&Gel::OnGitFinished, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4));

  int p1 = proc_runner_.Run({"git", "log", "--oneline", "--color=never"});
  int p2 = proc_runner_.Run({"git", "diff-tree", "-r", "-p", "--textconv",
                             "--submodule", "-C", "--cc", "--no-commit-id",
                             "-U3", "--root", "HEAD"});
  int p3 = proc_runner_.Run({"git", "log", "--oneline", "--color=never"});
  int p4 = proc_runner_.Run({"git", "diff-tree", "-r", "-p", "--textconv",
                             "--submodule", "-C", "--cc", "--no-commit-id",
                             "-U3", "--root", "HEAD"});
  int p5 = proc_runner_.Run({"git", "log", "--oneline", "--color=never"});
  [[maybe_unused]] int p6 = proc_runner_.Run(
      {"git", "diff-tree", "-r", "-p", "--textconv", "--submodule", "-C",
       "--cc", "--no-commit-id", "-U3", "--root", "HEAD"});

  proc_runner_.Kill(p1);
  proc_runner_.Kill(p2);

  // proc_runner_.Run({"git", "log", "--color=never"});
  // proc_runner_.Run({"git", "diff-tree", "-r", "-p", "--textconv", "--submodule",
  //                   "-C", "--cc", "--no-commit-id", "-U3", "--root", "HEAD"});

  proc_runner_.Kill(p3);
  proc_runner_.Kill(p4);
  proc_runner_.Kill(p5);
  proc_runner_.Kill(p6);

  return true;
}

void Gel::Update(float delta_time) {
  // ImGui::ShowDemoWindow();

  // We demonstrate using the full viewport area or the work area (without
  // menu-bars, task-bars etc.) Based on your use case you may want one or the
  // other.
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Gel", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings)) {
    static ImGuiTableFlags flags =
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable;

    // When using ScrollX or ScrollY we need to specify a size for our table
    // container! Otherwise by default the table will fit all available space,
    // like a BeginChild() call.
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
    if (ImGui::BeginTable("table_scrolly", 3, flags, outer_size)) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_None);
      ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_None);
      ImGui::TableSetupColumn("Three", ImGuiTableColumnFlags_None);
      ImGui::TableHeadersRow();

      // Demonstrate using clipper for large vertical lists
      ImGuiListClipper clipper;
      clipper.Begin(1000);
      while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
          ImGui::TableNextRow();
          for (int column = 0; column < 3; column++) {
            ImGui::TableSetColumnIndex(column);
            ImGui::Text("Hello %d,%d", column, row);
          }
        }
      }
      ImGui::EndTable();
    }
  }
  ImGui::End();
}

void Gel::OnGitOutput(int pid, std::string line) {
  LOG(0) << "pid: " << pid << " output: " << line;
}

void Gel::OnGitFinished(int pid,
                        Exec::Status status,
                        int result,
                        std::string err) {
  LOG(0) << "Finished pid: " << pid << " status: " << static_cast<int>(status)
         << " result: " << result << " err: " << err;
}
