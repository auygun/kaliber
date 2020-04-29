#include <android_native_app_glue.h>
#include <memory>
#include <string>
#include "../base/file.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/renderer/renderer.h"
#include "../third_party/android/gestureDetector.h"
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

Platform::Platform() = default;
Platform::~Platform() = default;

ANativeWindow* Platform::GetNativeWindow() {
  return app_->window;
}

int32_t Platform::HandleInput(android_app* app, AInputEvent* event) {
  Platform* platform = reinterpret_cast<Platform*>(app->userData);

  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    ndk_helper::GESTURE_STATE doubleTapState =
        platform->doubletap_detector_->Detect(event);
    ndk_helper::GESTURE_STATE dragState = platform->drag_detector_->Detect(event);
    ndk_helper::GESTURE_STATE pinchState = platform->pinch_detector_->Detect(event);

    // Double tap detector has a priority over other detectors
    if (doubleTapState == ndk_helper::GESTURE_STATE_ACTION) {
      // Detect double tap
      // platform->tap_camera_.Reset(true);
      Vector2 v;
      platform->doubletap_detector_->GetPointer(v);
      v = engine::Engine::Get().ToPosition(v);
      LOG << "double-tap: " << v;
    } else {
      // Handle drag state
      if (dragState & ndk_helper::GESTURE_STATE_START) {
        // Otherwise, start dragging
        Vector2 v;
        platform->drag_detector_->GetPointer(v);
        v = engine::Engine::Get().ToPosition(v);
        // platform->tap_camera_.BeginDrag(v);
      LOG << "drag-start: " << v;
      } else if (dragState & ndk_helper::GESTURE_STATE_MOVE) {
        Vector2 v;
        platform->drag_detector_->GetPointer(v);
        v = engine::Engine::Get().ToPosition(v);
        // platform->tap_camera_.Drag(v);
      LOG << "drag: " << v;
      } else if (dragState & ndk_helper::GESTURE_STATE_END) {
        // platform->tap_camera_.EndDrag();
        LOG << "drag-end!";
      }

      // Handle pinch state
      if (pinchState & ndk_helper::GESTURE_STATE_START) {
        // Start new pinch
        Vector2 v1;
        Vector2 v2;
        platform->pinch_detector_->GetPointers(v1, v2);
        v1 = engine::Engine::Get().ToPosition(v1);
        v2 = engine::Engine::Get().ToPosition(v2);
        // platform->tap_camera_.BeginPinch(v1, v2);
        LOG << "pinch-start: " << v1 << " " << v2;
      } else if (pinchState & ndk_helper::GESTURE_STATE_MOVE) {
        // Multi touch
        // Start new pinch
        Vector2 v1;
        Vector2 v2;
        platform->pinch_detector_->GetPointers(v1, v2);
        v1 = engine::Engine::Get().ToPosition(v1);
        v2 = engine::Engine::Get().ToPosition(v2);
        // platform->tap_camera_.Pinch(v1, v2);
        LOG << "pinch: " << v1 << " " << v2;
      }
    }
    return 1;
  }
  return 0;
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

  doubletap_detector_ = std::make_unique<ndk_helper::DoubletapDetector>();
  drag_detector_ = std::make_unique<ndk_helper::DragDetector>();
  pinch_detector_ = std::make_unique<ndk_helper::PinchDetector>();

  doubletap_detector_->SetConfiguration(app_->config);
  drag_detector_->SetConfiguration(app_->config);
  pinch_detector_->SetConfiguration(app_->config);

  root_path_ = GetApkPath(app->activity);
  LOG << "Root path: " << root_path_.c_str();

  app->userData = reinterpret_cast<void*>(this);
  app->onAppCmd = Platform::HandleCmd;
  app->onInputEvent = Platform::HandleInput;

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
