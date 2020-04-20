#include "platform.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/renderer/renderer.h"
// #include <pthread.h>

// void PTreadWorkaround() {
//   int i = pthread_getconcurrency();
// };

void Platform::Initialize() {
  root_path_ = "../../assets/";
  LOG("Root path: %s\n", root_path_.c_str());
  if (!engine::Engine::Get().GetRenderer().Init()) {
    LOG("Failed to initialize the renderer.\n");
    throw internal_error;
  }
}

void Platform::Update() {
  should_exit_ = engine::Engine::Get().GetRenderer().ShouldExit();
}

int main(int argc, char **argv) {
  // PTreadWorkaround();
  Platform &platform = Platform::Get();
  try {
    platform.Initialize();
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (Platform::InternalError &e) {
    return -1;
  }
  return 0;
}
