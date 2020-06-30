#include "platform_android.h"

#include <android_native_app_glue.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "../../base/log.h"
#include "../../base/task_runner.h"
#include "../audio/audio.h"
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

void Vibrate(ANativeActivity* activity, int duration) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jclass context_clazz = env->FindClass("android/content/Context");

  jfieldID vibrator_service_id = env->GetStaticFieldID(
      context_clazz, "VIBRATOR_SERVICE", "Ljava/lang/String;");
  jobject vibrator_service_str =
      env->GetStaticObjectField(context_clazz, vibrator_service_id);

  jmethodID get_system_service_id =
      env->GetMethodID(activity_clazz, "getSystemService",
                       "(Ljava/lang/String;)Ljava/lang/Object;");
  jobject vibrator_service_obj = env->CallObjectMethod(
      activity->clazz, get_system_service_id, vibrator_service_str);

  jclass vibrator_service_clazz = env->GetObjectClass(vibrator_service_obj);
  jmethodID vibrate_id =
      env->GetMethodID(vibrator_service_clazz, "vibrate", "(J)V");

  jlong length = duration;
  env->CallVoidMethod(vibrator_service_obj, vibrate_id, length);

  env->DeleteLocalRef(vibrator_service_obj);
  env->DeleteLocalRef(vibrator_service_clazz);
  env->DeleteLocalRef(vibrator_service_str);
  env->DeleteLocalRef(context_clazz);
  env->DeleteLocalRef(activity_clazz);
  activity->vm->DetachCurrentThread();
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

PlatformAndroid::PlatformAndroid() = default;
PlatformAndroid::~PlatformAndroid() = default;

int32_t PlatformAndroid::HandleInput(android_app* app, AInputEvent* event) {
  PlatformAndroid* platform = reinterpret_cast<PlatformAndroid*>(app->userData);

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
  } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    int32_t action = AMotionEvent_getAction(event);
    int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                    AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    uint32_t flags = action & AMOTION_EVENT_ACTION_MASK;
    int32_t count = AMotionEvent_getPointerCount(event);
    int32_t pointer_id = AMotionEvent_getPointerId(event, index);
    Vector2 pos[2] = {platform->pointer_pos_[0], platform->pointer_pos_[1]};
    for (auto i = 0; i < count; ++i) {
      int32_t id = AMotionEvent_getPointerId(event, i);
      pos[id] = {AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)};
      pos[id] = platform->engine_->ToPosition(pos[id]);
    }

    if (pointer_id >= 2)
      return 0;

    std::unique_ptr<InputEvent> input_event;

    switch (flags) {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        DLOG << "AMOTION_EVENT_ACTION_DOWN - pointer_id: " << pointer_id;
        platform->pointer_pos_[pointer_id] = pos[pointer_id];
        platform->pointer_down_[pointer_id] = true;
        input_event =
            std::make_unique<InputEvent>(InputEvent::kDragStart, pointer_id,
                                         pos[pointer_id] * Vector2(1, -1));
        break;

      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
        DLOG << "AMOTION_EVENT_ACTION_UP -   pointer_id: " << pointer_id;
        platform->pointer_pos_[pointer_id] = pos[pointer_id];
        platform->pointer_down_[pointer_id] = false;
        input_event = std::make_unique<InputEvent>(
            InputEvent::kDragEnd, pointer_id, pos[pointer_id] * Vector2(1, -1));
        break;

      case AMOTION_EVENT_ACTION_MOVE:
        if (platform->pointer_down_[0] && pos[0] != platform->pointer_pos_[0]) {
          platform->pointer_pos_[0] = pos[0];
          input_event = std::make_unique<InputEvent>(InputEvent::kDrag, 0,
                                                     pos[0] * Vector2(1, -1));
        }
        if (platform->pointer_down_[1] && pos[1] != platform->pointer_pos_[1]) {
          platform->pointer_pos_[1] = pos[1];
          input_event = std::make_unique<InputEvent>(InputEvent::kDrag, 1,
                                                     pos[1] * Vector2(1, -1));
        }
        break;

      case AMOTION_EVENT_ACTION_CANCEL:
        input_event = std::make_unique<InputEvent>(InputEvent::kDragCancel);
        break;
    }

    if (input_event) {
      platform->engine_->AddInputEvent(std::move(input_event));
      return 1;
    }
  }

  return 0;
}

void PlatformAndroid::HandleCmd(android_app* app, int32_t cmd) {
  PlatformAndroid* platform = reinterpret_cast<PlatformAndroid*>(app->userData);

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

void PlatformAndroid::Initialize(android_app* app) {
  PlatformBase::Initialize();

  app_ = app;

  mobile_device_ = true;

  root_path_ = GetApkPath(app->activity);
  LOG << "Root path: " << root_path_.c_str();

  device_dpi_ = getDensityDpi(app);
  LOG << "Device DPI: " << device_dpi_;

  app->userData = reinterpret_cast<void*>(this);
  app->onAppCmd = PlatformAndroid::HandleCmd;
  app->onInputEvent = PlatformAndroid::HandleInput;

  Update();
}

void PlatformAndroid::Update() {
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

void PlatformAndroid::Exit() {
  ANativeActivity_finish(app_->activity);
}

void PlatformAndroid::Vibrate(int duration) {
  ::Vibrate(app_->activity, duration);
}

}  // namespace eng

void android_main(android_app* app) {
  eng::PlatformAndroid platform;
  try {
    platform.Initialize(app);
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (eng::PlatformBase::InternalError& e) {
  }
  _exit(0);
}
