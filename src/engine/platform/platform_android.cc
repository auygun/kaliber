#include "platform.h"

#include <android_native_app_glue.h>
#include <unistd.h>
#include <string>

#include "../../base/file.h"
#include "../../base/log.h"
#include "../../third_party/android/gestureDetector.h"
#include "../audio/audio_oboe.h"
#include "../engine.h"
#include "../input_event.h"
#include "../renderer/renderer.h"

using namespace base;

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
  activity->vm->DetachCurrentThread();

  return apk_path;
}

int32_t getDensityDpi(android_app* app) {
  AConfiguration* config = AConfiguration_new();
  AConfiguration_fromAssetManager(config, app->activity->assetManager);
  int32_t density = AConfiguration_getDensity(config);
  AConfiguration_delete(config);
  return density;
}

}  // namespace

namespace eng {

Platform::Platform() = default;
Platform::~Platform() = default;

int32_t Platform::HandleInput(android_app* app, AInputEvent* event) {
  Platform* platform = reinterpret_cast<Platform*>(app->userData);

  if (!platform->engine_)
    return 0;

  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY &&
      AKeyEvent_getKeyCode(event) == AKEYCODE_BACK) {
    if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP) {
      auto input_event =
          std::make_unique<InputEvent>(InputEvent::kNavigateBack);
      platform->engine_->AddInputEvent(std::move(input_event));
    }
    return 1;
  }

  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    ndk_helper::GESTURE_STATE tap_state =
        platform->tap_detector_->Detect(event);
    ndk_helper::GESTURE_STATE drag_state =
        platform->drag_detector_->Detect(event);
    ndk_helper::GESTURE_STATE pinch_state =
        platform->pinch_detector_->Detect(event);

    // Tap detector has a priority over other detectors
    if (tap_state == ndk_helper::GESTURE_STATE_ACTION) {
      platform->engine_->AddInputEvent(
          std::make_unique<InputEvent>(InputEvent::kDragCancel));
      // Detect tap
      Vector2 v;
      platform->tap_detector_->GetPointer(v);
      v = platform->engine_->ToPosition(v);
      // DLOG << "Tap: " << v;
      auto input_event =
          std::make_unique<InputEvent>(InputEvent::kTap, v * Vector2(1, -1));
      platform->engine_->AddInputEvent(std::move(input_event));
    } else {
      // Handle drag state
      if (drag_state & ndk_helper::GESTURE_STATE_START) {
        // Otherwise, start dragging
        Vector2 v;
        platform->drag_detector_->GetPointer(v);
        v = platform->engine_->ToPosition(v);
        // DLOG << "drag-start: " << v;
        auto input_event = std::make_unique<InputEvent>(InputEvent::kDragStart,
                                                        v * Vector2(1, -1));
        platform->engine_->AddInputEvent(std::move(input_event));
      } else if (drag_state & ndk_helper::GESTURE_STATE_MOVE) {
        Vector2 v;
        platform->drag_detector_->GetPointer(v);
        v = platform->engine_->ToPosition(v);
        // DLOG << "drag: " << v;
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kDrag, v * Vector2(1, -1));
        platform->engine_->AddInputEvent(std::move(input_event));
      } else if (drag_state & ndk_helper::GESTURE_STATE_END) {
        // DLOG << "drag-end!";
        auto input_event = std::make_unique<InputEvent>(InputEvent::kDragEnd);
        platform->engine_->AddInputEvent(std::move(input_event));
      }

      // Handle pinch state
      if (pinch_state & ndk_helper::GESTURE_STATE_START) {
        platform->engine_->AddInputEvent(
            std::make_unique<InputEvent>(InputEvent::kDragCancel));
        // Start new pinch
        Vector2 v1;
        Vector2 v2;
        platform->pinch_detector_->GetPointers(v1, v2);
        v1 = platform->engine_->ToPosition(v1);
        v2 = platform->engine_->ToPosition(v2);
        // DLOG << "pinch-start: " << v1 << " " << v2;
        auto input_event = std::make_unique<InputEvent>(
            InputEvent::kPinchStart, v1 * Vector2(1, -1), v2 * Vector2(1, -1));
        platform->engine_->AddInputEvent(std::move(input_event));
      } else if (pinch_state & ndk_helper::GESTURE_STATE_MOVE) {
        // Multi touch
        // Start new pinch
        Vector2 v1;
        Vector2 v2;
        platform->pinch_detector_->GetPointers(v1, v2);
        v1 = platform->engine_->ToPosition(v1);
        v2 = platform->engine_->ToPosition(v2);
        // DLOG << "pinch: " << v1 << " " << v2;
        auto input_event = std::make_unique<InputEvent>(
            InputEvent::kPinch, v1 * Vector2(1, -1), v2 * Vector2(1, -1));
        platform->engine_->AddInputEvent(std::move(input_event));
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
      DLOG << "APP_CMD_INIT_WINDOW";
      if (app->window != NULL) {
        if (!platform->renderer_->Initialize(app->window)) {
          LOG << "Failed to initialize the renderer.";
          throw internal_error;
        }
      }
      break;

    case APP_CMD_TERM_WINDOW:
      DLOG << "APP_CMD_TERM_WINDOW";
      platform->renderer_->Shutdown();
      break;

    case APP_CMD_CONFIG_CHANGED:
      DLOG << "APP_CMD_CONFIG_CHANGED";
      if (platform->app_->window != NULL) {
        int width = platform->renderer_->screen_width();
        int height = platform->renderer_->screen_height();
        if (width != ANativeWindow_getWidth(app->window) ||
            height != ANativeWindow_getHeight(app->window)) {
          platform->renderer_->Shutdown();
          if (!platform->renderer_->Initialize(platform->app_->window)) {
            LOG << "Failed to initialize the renderer.";
            throw internal_error;
          }
        }
      }
      break;

    case APP_CMD_STOP:
      DLOG << "APP_CMD_STOP";
      break;

    case APP_CMD_GAINED_FOCUS:
      DLOG << "APP_CMD_GAINED_FOCUS";
      platform->timer_.Reset();
      platform->has_focus_ = true;
      if (platform->engine_)
        platform->engine_->GainedFocus();
      break;

    case APP_CMD_LOST_FOCUS:
      DLOG << "APP_CMD_LOST_FOCUS";
      platform->has_focus_ = false;
      if (platform->engine_)
        platform->engine_->LostFocus();
      break;

    case APP_CMD_LOW_MEMORY:
      DLOG << "APP_CMD_LOW_MEMORY";
      break;
  }
}

void Platform::Initialize(android_app* app) {
  LOG << "Initializing platform.";
  app_ = app;

  audio_ = std::make_unique<AudioOboe>();
  if (!audio_->Initialize()) {
    LOG << "Failed to initialize audio system.";
    throw internal_error;
  }

  renderer_ = std::make_unique<Renderer>();

  tap_detector_ = std::make_unique<ndk_helper::TapDetector>();
  drag_detector_ = std::make_unique<ndk_helper::DragDetector>();
  pinch_detector_ = std::make_unique<ndk_helper::PinchDetector>();

  tap_detector_->SetConfiguration(app_->config);
  drag_detector_->SetConfiguration(app_->config);
  pinch_detector_->SetConfiguration(app_->config);

  mobile_device_ = true;

  root_path_ = GetApkPath(app->activity);
  LOG << "Root path: " << root_path_.c_str();

  device_dpi_ = getDensityDpi(app);
  LOG << "Device DPI: " << device_dpi_;

  app->userData = reinterpret_cast<void*>(this);
  app->onAppCmd = Platform::HandleCmd;
  app->onInputEvent = Platform::HandleInput;

  Update();
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
      LOG << "App destroy requested.";
      should_exit_ = true;
      break;
    }
    if (has_focus_)
      break;
  }
}

void Platform::Exit() {
  ANativeActivity_finish(app_->activity);
}

}  // namespace eng

void android_main(android_app* app) {
  eng::Platform platform;
  try {
    platform.Initialize(app);
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (eng::Platform::InternalError& e) {
  }
  _exit(0);
}
