#include "engine/platform/platform.h"

#include <combaseapi.h>

#include "base/log.h"
#include "base/vecmath.h"
#include "engine/input_event.h"
#include "engine/platform/platform_observer.h"

using namespace base;

namespace eng {

void KaliberMain(Platform* platform);

Platform::Platform(HINSTANCE instance, int cmd_show)
    : instance_(instance), cmd_show_(cmd_show) {
  LOG(0) << "Initializing platform.";

  root_path_ = ".\\";
  data_path_ = ".\\";
  shared_data_path_ = ".\\";

  char dest[MAX_PATH];
  memset(dest, 0, sizeof(dest));
  if (GetModuleFileNameA(NULL, dest, MAX_PATH) > 0) {
    std::string path = dest;
    std::size_t last_slash_pos = path.find_last_of('\\');
    if (last_slash_pos != std::string::npos)
      path = path.substr(0, last_slash_pos + 1);

    root_path_ = path;
    data_path_ = path;
    shared_data_path_ = path;
  }

  LOG(0) << "Root path: " << root_path_.c_str();
  LOG(0) << "Data path: " << data_path_.c_str();
  LOG(0) << "Shared data path: " << shared_data_path_.c_str();

  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  CHECK(SUCCEEDED(hr)) << "Unable to initialize COM: " << hr;

  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = instance_;
  wcex.hIcon = nullptr;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = L"KaliberWndClass";
  wcex.hIconSm = nullptr;
  RegisterClassEx(&wcex);
}

void Platform::CreateMainWindow() {
  wnd_ = CreateWindow(L"KaliberWndClass", L"Kaliber", WS_OVERLAPPEDWINDOW,
                      CW_USEDEFAULT, 0, 800, 1205, nullptr, nullptr, instance_,
                      this);
  CHECK(wnd_);

  ShowWindow(wnd_, cmd_show_);
  UpdateWindow(wnd_);
}

Platform::~Platform() {
  LOG(0) << "Shutting down platform.";
}

void Platform::Update() {
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      should_exit_ = true;
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void Platform::Exit() {
  should_exit_ = true;
}

void Platform::Vibrate(int duration) {}

void Platform::ShowInterstitialAd() {}

void Platform::ShareFile(const std::string& file_name) {}

void Platform::SetKeepScreenOn(bool keep_screen_on) {}

HINSTANCE Platform::GetInstance() {
  return instance_;
}

HWND Platform::GetWindow() {
  return wnd_;
}

LRESULT CALLBACK Platform::WndProc(HWND wnd,
                                   UINT message,
                                   WPARAM wparam,
                                   LPARAM lparam) {
  auto* platform =
      reinterpret_cast<Platform*>(GetWindowLongPtr(wnd, GWL_USERDATA));

  switch (message) {
    case WM_CREATE:
      SetWindowLongPtr(wnd, GWL_USERDATA,
                       (LONG_PTR)(((LPCREATESTRUCT)lparam)->lpCreateParams));
      break;
    case WM_SIZE:
      platform->observer_->OnWindowResized(LOWORD(lparam), HIWORD(lparam));
      break;
    case WM_DESTROY:
      platform->observer_->OnWindowDestroyed();
      PostQuitMessage(0);
      break;
    case WM_ACTIVATEAPP:
      if (wparam == TRUE)
        platform->observer_->GainedFocus(false);
      else
        platform->observer_->LostFocus();
      break;
    case WM_MOUSEMOVE:
      if (wparam == MK_LBUTTON) {
        Vector2f v(MAKEPOINTS(lparam).x, MAKEPOINTS(lparam).y);
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kDrag, 0, v);
        platform->observer_->AddInputEvent(std::move(input_event));
      }
      break;
    case WM_LBUTTONDOWN: {
      Vector2f v(MAKEPOINTS(lparam).x, MAKEPOINTS(lparam).y);
      auto input_event =
          std::make_unique<InputEvent>(InputEvent::kDragStart, 0, v);
      platform->observer_->AddInputEvent(std::move(input_event));
    } break;
    case WM_LBUTTONUP: {
      Vector2f v(MAKEPOINTS(lparam).x, MAKEPOINTS(lparam).y);
      auto input_event =
          std::make_unique<InputEvent>(InputEvent::kDragEnd, 0, v);
      platform->observer_->AddInputEvent(std::move(input_event));
    } break;
    default:
      return DefWindowProc(wnd, message, wparam, lparam);
  }
  return 0;
}

}  // namespace eng

int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE prev_instance,
                   PSTR cmd_line,
                   int cmd_show) {
  eng::Platform platform(instance, cmd_show);
  eng::KaliberMain(&platform);
  return 0;
}
