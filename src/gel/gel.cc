#include "gel/gel.h"

#include "base/closure.h"
#include "base/random.h"
#include "base/vecmath.h"
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
  static base::Closure fade_out;
  static base::Closure fade_in = [&]() -> void {
    animator_.SetBlending(
        Vector4f(Engine::Get().GetRandomGenerator().Rand(),
                 Engine::Get().GetRandomGenerator().Rand(),
                 Engine::Get().GetRandomGenerator().Rand(), 1),
        1.0f);
    animator_.SetEndCallback(Animator::kBlending, fade_out);
    animator_.Play(Animator::kBlending, false);
  };
  fade_out = [&]() -> void {
    animator_.SetBlending(Vector4f(0, 0, 0, 1), 1.0f);
    animator_.SetEndCallback(Animator::kBlending, fade_in);
    animator_.Play(Animator::kBlending, false);
  };

  quad_.SetSize(Vector2f(0.1f, 0.6f));
  // quad_.SetVisible(true);
  quad_.SetColor(Vector4f(0, 0, 1, 1));
  animator_.Attach(&quad_);
  // fade_in();

  // animator_.SetRotation(PI2f, 2.0);
  // animator_.Play(Animator::kRotation, true);

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

  proc_runner_.Abort(p1);
  proc_runner_.Abort(p2);

  proc_runner_.Run({"git", "log", "--oneline", "--color=never"});
  proc_runner_.Run({"git", "diff-tree", "-r", "-p", "--textconv", "--submodule",
                    "-C", "--cc", "--no-commit-id", "-U3", "--root", "HEAD"});

  proc_runner_.Abort(p3);
  proc_runner_.Abort(p4);
  proc_runner_.Abort(p5);
  // proc_runner_.Abort(p6);

  return true;
}

void Gel::Update(float delta_time) {
  ImGui::ShowDemoWindow();
}

void Gel::ContextLost() {}

void Gel::LostFocus() {}

void Gel::GainedFocus(bool from_interstitial_ad) {}

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
