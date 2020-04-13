#if defined(__linux__)

#include "../engine/engine.h"
// #include "window.h"
// #include <pthread.h>

// void PTreadWorkaround() {
//   int i = pthread_getconcurrency();
// };

int main(int argc, char **argv) {
  // PTreadWorkaround();

  // int screenWidth   = 480,
  //     screenHeight  = 800;

  // if (!SetupOpenGLWindow(screenWidth, screenHeight))
  //   return -1;

  // Engine &engine = Engine::Get();
  // engine.Init(screenWidth, screenHeight);

  // for (;;) {
  //   engine.Update();
  //   UpdateOpenGLWindow();
  // }

  // ShutdownOpenGLWindow();
  // return 0;

  return engine::Engine::Run();
}

#endif // __linux__
