#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>
#include <exception>

#if defined(__ANDROID__)
struct android_app;
struct ANativeWindow;
#endif

class Platform {
 public:
  Platform() = default;
  ~Platform() = default;

  static Platform& Get();

#if defined(__ANDROID__)
  void Initialize(android_app *app);
#elif defined(__linux__)
  void Initialize();
#endif

  void Shutdown();

  void Update();

  void RunMainLoop();

  bool ShouldExit() { return should_exit_; }

  bool HasFocus() { return has_focus_; }

  const std::string& GetRootPath() { return root_path_; }

#if defined(__ANDROID__)
  ANativeWindow* GetNativeWindow();
#endif

  static class InternalError : public std::exception {} internal_error;

 private:
  std::string root_path_;
  bool has_focus_ = false;
  bool should_exit_ = false;

#if defined(__ANDROID__)
  android_app *app_ = nullptr;

  static void HandleCmd(android_app* app, int32_t cmd);
#endif
};

#endif // PLATFORM_H
