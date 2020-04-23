#include <android_native_app_glue.h>
#include <memory>
#include <string>
#include "../base/file.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/renderer/renderer.h"
#include "platform.h"

namespace {

std::string GetApkPath(ANativeActivity* activity) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID get_application_info_id =
      env->GetMethodID(activity_clazz, "getApplicationInfo",
                       "()Landroid/content/pm/ApplicationInfo;");
  jobject app_info_obj =
      env->CallObjectMethod(activity->clazz, get_application_info_id);

  jclass app_info_clazz = env->GetObjectClass(app_info_obj);
  jfieldID source_dir_id =
      env->GetFieldID(app_info_clazz, "sourceDir", "Ljava/lang/String;");
  jstring source_dir_obj =
      (jstring)env->GetObjectField(app_info_obj, source_dir_id);

  const char* source_dir = env->GetStringUTFChars(source_dir_obj, nullptr);
  std::string apk_path = std::string(source_dir);

  env->ReleaseStringUTFChars(source_dir_obj, source_dir);
  env->DeleteLocalRef(app_info_clazz);
  env->DeleteLocalRef(activity_clazz);

  return apk_path;
}

}  // namespace

ANativeWindow* Platform::GetNativeWindow() {
  return app_->window;
}

void Platform::HandleCmd(android_app* app, int32_t cmd) {
  Platform* platform = reinterpret_cast<Platform*>(app->userData);
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      break;
    case APP_CMD_INIT_WINDOW:
      if (app->window != NULL) {
        if (!engine::Engine::Get().GetRenderer().StartWorker()) {
          LOG << "Failed to initialize the renderer.";
          throw internal_error;
        }
        platform->has_focus_ = true;
      }
      break;
    case APP_CMD_TERM_WINDOW:
      engine::Engine::Get().GetRenderer().TerminateWorker();
      platform->has_focus_ = false;
      break;
    case APP_CMD_STOP:
      break;
    case APP_CMD_GAINED_FOCUS:
      // eng->ResumeSensors();
      platform->has_focus_ = true;
      break;
    case APP_CMD_LOST_FOCUS:
      // eng->SuspendSensors();
      platform->has_focus_ = false;
      break;
    case APP_CMD_LOW_MEMORY:
      engine::Engine::Get().TrimMemory();
      break;
  }
}

void Platform::Initialize(android_app* app) {
  app_ = app;
  root_path_ = GetApkPath(app->activity);
  LOG << "Root path: " << root_path_.c_str();

  app->userData = reinterpret_cast<void*>(this);
  app->onAppCmd = Platform::HandleCmd;

  Update();
}

void Platform::Shutdown() {
  engine::Engine::Get().GetRenderer().TerminateWorker();
}

void Platform::Update() {
  int id;
  int events;
  android_poll_source* source;

  while ((id = ALooper_pollAll(has_focus_ ? 0 : -1, NULL, &events,
                               (void**)&source)) >= 0) {
    if (source != NULL)
      source->process(app_, source);
    if (app_->destroyRequested != 0) {
      should_exit_ = true;
      break;
    }
    if (has_focus_)
      break;
  }
}

void android_main(android_app* app) {
  Platform& platform = Platform::Get();
  try {
    platform.Initialize(app);
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (Platform::InternalError& e) {
  }
}
