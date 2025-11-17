#include "engine/platform/platform.h"

#include <combaseapi.h>

#include "base/log.h"
#include "base/vecmath.h"
#include "engine/input_event.h"
#include "engine/platform/platform_observer.h"

using namespace base;

namespace eng {

namespace {

Key TranslateWin32Key(WPARAM wparam) {
  switch (wparam) {
    case 'A':
      return Key::A;
    case 'B':
      return Key::B;
    case 'C':
      return Key::C;
    case 'D':
      return Key::D;
    case 'E':
      return Key::E;
    case 'F':
      return Key::F;
    case 'G':
      return Key::G;
    case 'H':
      return Key::H;
    case 'I':
      return Key::I;
    case 'J':
      return Key::J;
    case 'K':
      return Key::K;
    case 'L':
      return Key::L;
    case 'M':
      return Key::M;
    case 'N':
      return Key::N;
    case 'O':
      return Key::O;
    case 'P':
      return Key::P;
    case 'Q':
      return Key::Q;
    case 'R':
      return Key::R;
    case 'S':
      return Key::S;
    case 'T':
      return Key::T;
    case 'U':
      return Key::U;
    case 'V':
      return Key::V;
    case 'W':
      return Key::W;
    case 'X':
      return Key::X;
    case 'Y':
      return Key::Y;
    case 'Z':
      return Key::Z;

    case '0':
      return Key::Num0;
    case '1':
      return Key::Num1;
    case '2':
      return Key::Num2;
    case '3':
      return Key::Num3;
    case '4':
      return Key::Num4;
    case '5':
      return Key::Num5;
    case '6':
      return Key::Num6;
    case '7':
      return Key::Num7;
    case '8':
      return Key::Num8;
    case '9':
      return Key::Num9;

    case VK_F1:
      return Key::F1;
    case VK_F2:
      return Key::F2;
    case VK_F3:
      return Key::F3;
    case VK_F4:
      return Key::F4;
    case VK_F5:
      return Key::F5;
    case VK_F6:
      return Key::F6;
    case VK_F7:
      return Key::F7;
    case VK_F8:
      return Key::F8;
    case VK_F9:
      return Key::F9;
    case VK_F10:
      return Key::F10;
    case VK_F11:
      return Key::F11;
    case VK_F12:
      return Key::F12;

    case VK_ESCAPE:
      return Key::Escape;
    case VK_SPACE:
      return Key::Space;
    case VK_RETURN:
      return Key::Enter;
    case VK_TAB:
      return Key::Tab;
    case VK_BACK:
      return Key::Backspace;

    case VK_UP:
      return Key::Up;
    case VK_DOWN:
      return Key::Down;
    case VK_LEFT:
      return Key::Left;
    case VK_RIGHT:
      return Key::Right;
    case VK_PRIOR:
      return Key::PageUp;
    case VK_NEXT:
      return Key::PageDown;
    case VK_HOME:
      return Key::Home;
    case VK_END:
      return Key::End;
    case VK_INSERT:
      return Key::Insert;
    case VK_DELETE:
      return Key::Delete;

    case VK_LSHIFT:
      return Key::ShiftLeft;
    case VK_RSHIFT:
      return Key::ShiftRight;
    case VK_LCONTROL:
      return Key::ControlLeft;
    case VK_RCONTROL:
      return Key::ControlRight;
    case VK_LMENU:
      return Key::AltLeft;
    case VK_RMENU:
      return Key::AltRight;
    default:
      return Key::Unknown;
  }
}

}  // namespace

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
  input_characters_.clear();

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
    case WM_CREATE: {
      SetWindowLongPtr(wnd, GWL_USERDATA,
                       (LONG_PTR)(((LPCREATESTRUCT)lparam)->lpCreateParams));
    } break;

    case WM_SIZE: {
      platform->observer_->OnWindowResized(LOWORD(lparam), HIWORD(lparam));
    } break;

    case WM_DESTROY: {
      platform->observer_->OnWindowDestroyed();
      PostQuitMessage(0);
    } break;

    case WM_ACTIVATEAPP: {
      if (wparam == TRUE)
        platform->observer_->GainedFocus(false);
      else
        platform->observer_->LostFocus();
    } break;

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

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
      Key translated_key = TranslateWin32Key(wparam);
      if (translated_key != Key::Unknown) {
        platform->keys_down_[static_cast<int>(translated_key)] = true;
      }
      break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP: {
      Key translated_key = TranslateWin32Key(wparam);
      if (translated_key != Key::Unknown) {
        platform->keys_down_[static_cast<int>(translated_key)] = false;
      }
      break;
    }
    case WM_CHAR: {
      // This is the Windows equivalent of XLookupString
      // wparam is the character (UTF-16)
      if (wparam > 0 && wparam < 0x10000) {
        platform->input_characters_.push_back(
            static_cast<unsigned int>(wparam));
      }
      break;
    }

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
