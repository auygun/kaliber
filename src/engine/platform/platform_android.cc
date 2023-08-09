#include "engine/platform/platform.h"

#include <memory>

#include <android_native_app_glue.h>
#include <dlfcn.h>
#include <jni.h>
#include <unistd.h>

#include "base/log.h"
#include "engine/input_event.h"
#include "engine/platform/platform_observer.h"

using namespace base;

namespace {

bool g_showing_interstitial_ad = false;

extern "C" {

JNIEXPORT void JNICALL
Java_com_kaliber_base_KaliberActivity_onShowAdResult(JNIEnv* env,
                                                     jobject obj,
                                                     jboolean succeeded) {
  g_showing_interstitial_ad = !!succeeded;
}
}

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

std::string GetDataPath(ANativeActivity* activity) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID get_dir_id = env->GetMethodID(
      activity_clazz, "getDir", "(Ljava/lang/String;I)Ljava/io/File;");
  jstring suffix = env->NewStringUTF("kaliber");
  jobject data_dir_obj = env->CallObjectMethod(activity->clazz, get_dir_id,
                                               suffix, 0 /* MODE_PRIVATE */);

  jclass file_clazz = env->FindClass("java/io/File");
  jmethodID get_absolute_path_id =
      env->GetMethodID(file_clazz, "getAbsolutePath", "()Ljava/lang/String;");
  jstring data_path_obj =
      (jstring)env->CallObjectMethod(data_dir_obj, get_absolute_path_id);

  const char* tmp = env->GetStringUTFChars(data_path_obj, nullptr);
  std::string data_path = std::string(tmp);

  env->ReleaseStringUTFChars(data_path_obj, tmp);
  env->DeleteLocalRef(activity_clazz);
  env->DeleteLocalRef(file_clazz);
  env->DeleteLocalRef(suffix);
  activity->vm->DetachCurrentThread();

  if (data_path.back() != '/')
    data_path += '/';
  return data_path;
}

std::string GetSharedDataPath(ANativeActivity* activity) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID get_dir_id = env->GetMethodID(activity_clazz, "getExternalFilesDir",
                                          "(Ljava/lang/String;)Ljava/io/File;");
  jobject data_dir_obj =
      env->CallObjectMethod(activity->clazz, get_dir_id, nullptr);

  jclass file_clazz = env->FindClass("java/io/File");
  jmethodID get_absolute_path_id =
      env->GetMethodID(file_clazz, "getAbsolutePath", "()Ljava/lang/String;");
  jstring data_path_obj =
      (jstring)env->CallObjectMethod(data_dir_obj, get_absolute_path_id);

  const char* tmp = env->GetStringUTFChars(data_path_obj, nullptr);
  std::string data_path = std::string(tmp);

  env->ReleaseStringUTFChars(data_path_obj, tmp);
  env->DeleteLocalRef(activity_clazz);
  env->DeleteLocalRef(file_clazz);
  activity->vm->DetachCurrentThread();

  if (data_path.back() != '/')
    data_path += '/';
  return data_path;
}

void ShowInterstitialAd(ANativeActivity* activity) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID show_interstitial_ad =
      env->GetMethodID(activity_clazz, "showInterstitialAd", "()V");
  env->CallVoidMethod(activity->clazz, show_interstitial_ad);

  env->DeleteLocalRef(activity_clazz);
  activity->vm->DetachCurrentThread();
}

void ShareFile(ANativeActivity* activity, const std::string& file_name) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID show_interstitial_ad =
      env->GetMethodID(activity_clazz, "shareFile", "(Ljava/lang/String;)V");
  jstring file_name_js = env->NewStringUTF(file_name.c_str());
  env->CallVoidMethod(activity->clazz, show_interstitial_ad, file_name_js);

  env->DeleteLocalRef(activity_clazz);
  env->DeleteLocalRef(file_name_js);
  activity->vm->DetachCurrentThread();
}

void SetKeepScreenOn(ANativeActivity* activity, bool keep_screen_on) {
  JNIEnv* env = nullptr;
  activity->vm->AttachCurrentThread(&env, nullptr);

  jclass activity_clazz = env->GetObjectClass(activity->clazz);
  jmethodID method_id =
      env->GetMethodID(activity_clazz, "setKeepScreenOn", "(Z)V");
  env->CallVoidMethod(activity->clazz, method_id, (jboolean)keep_screen_on);

  env->DeleteLocalRef(activity_clazz);
  activity->vm->DetachCurrentThread();
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

int32_t GetDensityDpi(android_app* app) {
  AConfiguration* config = AConfiguration_new();
  AConfiguration_fromAssetManager(config, app->activity->assetManager);
  int32_t density = AConfiguration_getDensity(config);
  AConfiguration_delete(config);
  return density;
}

}  // namespace

namespace eng {

void KaliberMain(Platform* platform);

int32_t Platform::HandleInput(android_app* app, AInputEvent* event) {
  Platform* platform = reinterpret_cast<Platform*>(app->userData);

  if (!platform->observer_)
    return 0;

  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY &&
      AKeyEvent_getKeyCode(event) == AKEYCODE_BACK) {
    if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP) {
      auto input_event =
          std::make_unique<InputEvent>(InputEvent::kNavigateBack);
      platform->observer_->AddInputEvent(std::move(input_event));
    }
    return 1;
  } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    int32_t action = AMotionEvent_getAction(event);
    int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                    AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    uint32_t flags = action & AMOTION_EVENT_ACTION_MASK;
    int32_t count = AMotionEvent_getPointerCount(event);
    int32_t pointer_id = AMotionEvent_getPointerId(event, index);
    Vector2f pos[2] = {platform->pointer_pos_[0], platform->pointer_pos_[1]};
    for (auto i = 0; i < count; ++i) {
      int32_t id = AMotionEvent_getPointerId(event, i);
      if (id < 2) {
        pos[id] = {AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)};
      }
    }

    if (pointer_id >= 2)
      return 0;

    std::unique_ptr<InputEvent> input_event;

    switch (flags) {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        platform->pointer_pos_[pointer_id] = pos[pointer_id];
        platform->pointer_down_[pointer_id] = true;
        input_event = std::make_unique<InputEvent>(InputEvent::kDragStart,
                                                   pointer_id, pos[pointer_id]);
        break;

      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
        platform->pointer_pos_[pointer_id] = pos[pointer_id];
        platform->pointer_down_[pointer_id] = false;
        input_event = std::make_unique<InputEvent>(InputEvent::kDragEnd,
                                                   pointer_id, pos[pointer_id]);
        break;

      case AMOTION_EVENT_ACTION_MOVE:
        if (platform->pointer_down_[0] && pos[0] != platform->pointer_pos_[0]) {
          platform->pointer_pos_[0] = pos[0];
          input_event =
              std::make_unique<InputEvent>(InputEvent::kDrag, 0, pos[0]);
        }
        if (platform->pointer_down_[1] && pos[1] != platform->pointer_pos_[1]) {
          platform->pointer_pos_[1] = pos[1];
          input_event =
              std::make_unique<InputEvent>(InputEvent::kDrag, 1, pos[1]);
        }
        break;

      case AMOTION_EVENT_ACTION_CANCEL:
        input_event = std::make_unique<InputEvent>(InputEvent::kDragCancel);
        break;
    }

    if (input_event) {
      platform->observer_->AddInputEvent(std::move(input_event));
      return 1;
    }
  }

  return 0;
}

void Platform::HandleCmd(android_app* app, int32_t cmd) {
  Platform* platform = reinterpret_cast<Platform*>(app->userData);

  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      break;

    case APP_CMD_INIT_WINDOW:
      DLOG(0) << "APP_CMD_INIT_WINDOW";
      if (app->window != NULL) {
        platform->SetFrameRate(60);
        platform->observer_->OnWindowCreated();
      }
      break;

    case APP_CMD_TERM_WINDOW:
      DLOG(0) << "APP_CMD_TERM_WINDOW";
      platform->observer_->OnWindowDestroyed();
      break;

    case APP_CMD_CONFIG_CHANGED:
      DLOG(0) << "APP_CMD_CONFIG_CHANGED";
      if (platform->app_->window != NULL)
        platform->observer_->OnWindowResized(
            ANativeWindow_getWidth(app->window),
            ANativeWindow_getHeight(app->window));
      break;

    case APP_CMD_STOP:
      DLOG(0) << "APP_CMD_STOP";
      break;

    case APP_CMD_GAINED_FOCUS:
      DLOG(0) << "APP_CMD_GAINED_FOCUS";
      // platform->timer_.Reset();
      platform->has_focus_ = true;
      platform->observer_->GainedFocus(g_showing_interstitial_ad);
      g_showing_interstitial_ad = false;
      break;

    case APP_CMD_LOST_FOCUS:
      DLOG(0) << "APP_CMD_LOST_FOCUS";
      platform->has_focus_ = false;
      platform->observer_->LostFocus();
      break;

    case APP_CMD_LOW_MEMORY:
      DLOG(0) << "APP_CMD_LOW_MEMORY";
      break;
  }
}

Platform::Platform(android_app* app) {
  LOG(0) << "Initializing platform.";

  app_ = app;
  mobile_device_ = true;

  root_path_ = ::GetApkPath(app->activity);
  LOG(0) << "Root path: " << root_path_.c_str();

  data_path_ = ::GetDataPath(app->activity);
  LOG(0) << "Data path: " << data_path_.c_str();

  shared_data_path_ = ::GetSharedDataPath(app->activity);
  LOG(0) << "Shared data path: " << shared_data_path_.c_str();

  device_dpi_ = ::GetDensityDpi(app);
  LOG(0) << "Device DPI: " << device_dpi_;

  app->userData = reinterpret_cast<void*>(this);
  app->onAppCmd = Platform::HandleCmd;
  app->onInputEvent = Platform::HandleInput;

  // Get pointers for functions that are supported from API > minSdk if
  // available.
  void* mLibAndroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
  if (mLibAndroid) {
    ANativeWindow_setFrameRate =
        reinterpret_cast<PFN_ANativeWindow_setFrameRate>(
            dlsym(mLibAndroid, "ANativeWindow_setFrameRate"));
    ANativeWindow_setFrameRateWithChangeStrategy =
        reinterpret_cast<PFN_ANativeWindow_setFrameRateWithChangeStrategy>(
            dlsym(mLibAndroid, "ANativeWindow_setFrameRateWithChangeStrategy"));
  }
}

void Platform::CreateMainWindow() {
  DCHECK(!app_->window);
  Update();
  DCHECK(app_->window);
}

Platform::~Platform() {
  LOG(0) << "Shutting down platform.";
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
      LOG(0) << "App destroy requested.";
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

void Platform::Vibrate(int duration) {
  ::Vibrate(app_->activity, duration);
}

void Platform::ShowInterstitialAd() {
  ::ShowInterstitialAd(app_->activity);
}

void Platform::ShareFile(const std::string& file_name) {
  ::ShareFile(app_->activity, file_name);
}

void Platform::SetKeepScreenOn(bool keep_screen_on) {
  ::SetKeepScreenOn(app_->activity, keep_screen_on);
}

void Platform::SetFrameRate(float frame_rate) {
  if (ANativeWindow_setFrameRateWithChangeStrategy) {
    ANativeWindow_setFrameRateWithChangeStrategy(
        app_->window, frame_rate,
        ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT, 1);
  } else if (ANativeWindow_setFrameRate) {
    ANativeWindow_setFrameRate(app_->window, frame_rate,
                               ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT);
  }
}

ANativeWindow* Platform::GetWindow() {
  return app_->window;
}

}  // namespace eng

void android_main(android_app* app) {
  eng::Platform platform(app);
  eng::KaliberMain(&platform);
  _exit(0);
}
