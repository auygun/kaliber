#include "../base/log.h"
#include "../base/file.h"
#include "../engine/engine.h"
#include "../engine/renderer/renderer.h"
#include "../engine/game.h"
#include "../engine/game_factory.h"
#include <android_native_app_glue.h>
#include <string>

namespace {

// class PlatformAndroid {
//   PlatformAndroid() = default;
//   ~PlatformAndroid() = default;

//   PlatformAndroid& PlatformAndroid::Get() {
//     static PlatformAndroid platform;
//     return platform;
//   }


// };

bool initialized = false;

void HandleCmd(android_app* app, int32_t cmd) {
  // engine::Engine* eng = reinterpret_cast<Engine*>(app->userData);
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      break;
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      if (app->window != NULL) {
        engine::Engine::Get().Init(app->window);
        initialized = true;
        // eng->has_focus_ = true;
        // eng->DrawFrame();
      }
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      engine::Engine::Get().Shutdown();
      // eng->has_focus_ = false;
      break;
    case APP_CMD_STOP:
      break;
    case APP_CMD_GAINED_FOCUS:
      // eng->ResumeSensors();
      // Start animation
      // eng->has_focus_ = true;
      break;
    case APP_CMD_LOST_FOCUS:
      // eng->SuspendSensors();
      // Also stop animating.
      // eng->has_focus_ = false;
      // eng->DrawFrame();
      break;
    case APP_CMD_LOW_MEMORY:
      // Free up GL resources
      // eng->TrimMemory();
      break;
  }
}

std::string GetApkPath(ANativeActivity* activity) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID get_application_info_id = env->GetMethodID(activity_clazz,
                                                  "getApplicationInfo",
                                                  "()Landroid/content/pm/ApplicationInfo;");
  jobject app_info_obj = env->CallObjectMethod(activity->clazz, get_application_info_id);

  jclass app_info_clazz = env->GetObjectClass(app_info_obj);
  jfieldID source_dir_id = env->GetFieldID(app_info_clazz, "sourceDir", "Ljava/lang/String;");
  jstring source_dir_obj = (jstring)env->GetObjectField(app_info_obj, source_dir_id);

  const char *source_dir = env->GetStringUTFChars(source_dir_obj, nullptr);
  std::string apk_path = std::string(source_dir);

  env->ReleaseStringUTFChars(source_dir_obj, source_dir);
  env->DeleteLocalRef(app_info_clazz);
  env->DeleteLocalRef(activity_clazz);

  return apk_path;
}

} // namespace

void android_main(android_app* state) {

  std::string apk_path = GetApkPath(state->activity);
  LOG("apk path: %s", apk_path.c_str());
  File::SetRootPath(apk_path.c_str());


  std::unique_ptr<engine::Game> game = engine::GameFactoryBase::CreateGame("");
  if (!game) {
    printf("No game found to run.\n");
    return;
  }


  // state->userData = reinterpret_cast<void*>(&engine::Engine::Get());
  state->onAppCmd = HandleCmd;

  int id;
  int events;
  android_poll_source* source;

  // If not animating, we will block forever waiting for events.
  // If animating, we loop until all events are read, then continue
  // to draw the next frame of animation.
  while ((id = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {
    if (source != NULL)
      source->process(state, source);
    if (initialized)
      break;
  }

  // if (!engine::Engine::Get().Init(state->window)) {
  //   printf("Failed to initialize the engine.\n");
  //   return;
  // }

  if (!game->Initialize()) {
    printf("Failed to initialize the game.\n");
    return;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;
  float last_time = engine::Engine::Get().GetTimer().GetSecondsAccumulated();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;)
  {
    engine::Engine::Get().Clear();
    game->Draw(frame_frac);
    engine::Engine::Get().Present();

    engine::Engine::Get().Update();

    float new_time = engine::Engine::Get().GetTimer().GetSecondsAccumulated();
    float frame_time = (new_time - last_time) * speed;
    last_time = new_time;
    accumulator += frame_time;

    // Subdivide the frame time.
    while (accumulator >= time_step)
    {
      game->Update(time_step);
      accumulator -= time_step;
    }

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;

    // if (m_platform->ShouldExit())
    //   break;
  }

  game->Shutdown();

  return;
}
