#include "engine/platform/platform.h"

#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <cstring>
#include <memory>

#include <X11/keysym.h>

#include "base/log.h"
#include "base/vecmath.h"
#include "engine/platform/platform_observer.h"

using namespace base;

namespace eng {

namespace {

// Maps X11's KeySym values to our platform-agnostic Key enum.
Key TranslateX11Key(KeySym key_sym) {
  switch (key_sym) {
    case XK_a:
    case XK_A:
      return Key::A;
    case XK_b:
    case XK_B:
      return Key::B;
    case XK_c:
    case XK_C:
      return Key::C;
    case XK_d:
    case XK_D:
      return Key::D;
    case XK_e:
    case XK_E:
      return Key::E;
    case XK_f:
    case XK_F:
      return Key::F;
    case XK_g:
    case XK_G:
      return Key::G;
    case XK_h:
    case XK_H:
      return Key::H;
    case XK_i:
    case XK_I:
      return Key::I;
    case XK_j:
    case XK_J:
      return Key::J;
    case XK_k:
    case XK_K:
      return Key::K;
    case XK_l:
    case XK_L:
      return Key::L;
    case XK_m:
    case XK_M:
      return Key::M;
    case XK_n:
    case XK_N:
      return Key::N;
    case XK_o:
    case XK_O:
      return Key::O;
    case XK_p:
    case XK_P:
      return Key::P;
    case XK_q:
    case XK_Q:
      return Key::Q;
    case XK_r:
    case XK_R:
      return Key::R;
    case XK_s:
    case XK_S:
      return Key::S;
    case XK_t:
    case XK_T:
      return Key::T;
    case XK_u:
    case XK_U:
      return Key::U;
    case XK_v:
    case XK_V:
      return Key::V;
    case XK_w:
    case XK_W:
      return Key::W;
    case XK_x:
    case XK_X:
      return Key::X;
    case XK_y:
    case XK_Y:
      return Key::Y;
    case XK_z:
    case XK_Z:
      return Key::Z;

    case XK_0:
      return Key::Num0;
    case XK_1:
      return Key::Num1;
    case XK_2:
      return Key::Num2;
    case XK_3:
      return Key::Num3;
    case XK_4:
      return Key::Num4;
    case XK_5:
      return Key::Num5;
    case XK_6:
      return Key::Num6;
    case XK_7:
      return Key::Num7;
    case XK_8:
      return Key::Num8;
    case XK_9:
      return Key::Num9;

    case XK_Escape:
      return Key::Escape;
    case XK_space:
      return Key::Space;
    case XK_Return:
      return Key::Enter;
    case XK_Tab:
      return Key::Tab;
    case XK_BackSpace:
      return Key::Backspace;

    case XK_Up:
      return Key::Up;
    case XK_Down:
      return Key::Down;
    case XK_Left:
      return Key::Left;
    case XK_Right:
      return Key::Right;

    case XK_Shift_L:
      return Key::ShiftLeft;
    case XK_Shift_R:
      return Key::ShiftRight;
    case XK_Control_L:
      return Key::ControlLeft;
    case XK_Control_R:
      return Key::ControlRight;
    case XK_Alt_L:
      return Key::AltLeft;
    case XK_Alt_R:
      return Key::AltRight;

      // ... (add more F-keys, Home, End, etc. as needed)

    default:
      return Key::Unknown;
  }
}

/**
 * @brief Decodes a raw UTF-8 byte stream into a vector of UTF-32 codepoints.
 * This implementation is platform-agnostic.
 *
 * @param buffer The raw character buffer (likely from XLookupString).
 * @param count The number of bytes in the buffer.
 * @param out_codepoints The vector to append UTF-32 codepoints to.
 */
static void DecodeUTF8ToUTF32(const char* buffer,
                              int count,
                              std::vector<unsigned int>& out_codepoints) {
  for (int i = 0; i < count;) {
    unsigned int codepoint = 0;
    int bytes_in_char = 0;
    unsigned char c = static_cast<unsigned char>(buffer[i]);

    if (c < 0x80) {  // 1-byte sequence (0..._
      codepoint = c;
      bytes_in_char = 1;
    } else if ((c & 0xE0) == 0xC0) {  // 2-byte sequence (110... 10......)
      if (i + 1 < count) {
        codepoint = ((c & 0x1F) << 6) | (buffer[i + 1] & 0x3F);
        bytes_in_char = 2;
      } else {
        i++;  // Incomplete sequence, skip
        continue;
      }
    } else if ((c & 0xF0) ==
               0xE0) {  // 3-byte sequence (1110... 10...... 10......)
      if (i + 2 < count) {
        codepoint = ((c & 0x0F) << 12) | ((buffer[i + 1] & 0x3F) << 6) |
                    (buffer[i + 2] & 0x3F);
        bytes_in_char = 3;
      } else {
        i++;  // Incomplete sequence, skip
        continue;
      }
    } else if ((c & 0xF8) ==
               0xF0) {  // 4-byte sequence (11110... 10...... 10...... 10......)
      if (i + 3 < count) {
        codepoint = ((c & 0x07) << 18) | ((buffer[i + 1] & 0x3F) << 12) |
                    ((buffer[i + 2] & 0x3F) << 6) | (buffer[i + 3] & 0x3F);
        bytes_in_char = 4;
      } else {
        i++;  // Incomplete sequence, skip
        continue;
      }
    } else {
      // Invalid/continuation byte, skip it
      i++;
      continue;
    }

    // Add valid, printable characters or tab
    if (codepoint >= 32 || codepoint == '\t') {
      out_codepoints.push_back(codepoint);
    }
    i += bytes_in_char;
  }
}

}  // namespace

void KaliberMain(Platform* platform);

Platform::Platform() {
  LOG(0) << "Initializing platform.";

  // This tells the C library to adopt the user's environment locale.
  // It's essential for XOpenIM to work correctly with UTF-8.
  setlocale(LC_CTYPE, "");
  // This tells X11 we will handle the input method.
  XSetLocaleModifiers("@im=none");

  root_path_ = "./";
  data_path_ = "./";
  shared_data_path_ = "./";

  char dest[PATH_MAX];
  memset(dest, 0, sizeof(dest));
  if (readlink("/proc/self/exe", dest, PATH_MAX) > 0) {
    std::string path = dest;
    std::size_t last_slash_pos = path.find_last_of('/');
    if (last_slash_pos != std::string::npos)
      path = path.substr(0, last_slash_pos + 1);

    root_path_ = path;
    data_path_ = path;
    shared_data_path_ = path;
  }

  LOG(0) << "Root path: " << root_path_.c_str();
  LOG(0) << "Data path: " << data_path_.c_str();
  LOG(0) << "Shared data path: " << shared_data_path_.c_str();
}

void Platform::CreateMainWindow() {
  bool res = CreateWindow(1920, 1080);
  CHECK(res) << "Failed to create window.";

  XSelectInput(display_, window_,
               KeyPressMask | KeyReleaseMask | PointerMotionMask |
                   Button1MotionMask | ButtonPressMask | ButtonReleaseMask |
                   FocusChangeMask | StructureNotifyMask);
  wm_delete_window_ = XInternAtom(display_, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display_, window_, &wm_delete_window_, 1);

  // Initialize X Input Method
  xim_ = XOpenIM(display_, NULL, NULL, NULL);
  if (!xim_) {
    LOG(0) << "Warning: XOpenIM failed. Non-ASCII input may not work.";
  } else {
    xic_ = XCreateIC(xim_, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                     XNClientWindow, window_, XNFocusWindow,
                     window_,  // Tell XIM which window to focus
                     NULL);
    if (!xic_) {
      LOG(0) << "Warning: XCreateIC failed. Non-ASCII input may not work.";
    }
  }
}

Platform::~Platform() {
  LOG(0) << "Shutting down platform.";

  // Clean up XIM
  if (xic_) {
    XDestroyIC(xic_);
    xic_ = 0;
  }
  if (xim_) {
    XCloseIM(xim_);
    xim_ = 0;
  }

  DestroyWindow();
}

void Platform::Update() {
  mouse_scroll_delta_ = 0.0f;
  input_characters_.clear();

  while (XPending(display_)) {
    XEvent e;
    XNextEvent(display_, &e);

    // This allows the IMEs to hook keypresses
    if (xic_ && XFilterEvent(&e, window_)) {
      continue;
    }

    switch (e.type) {
      case KeyPress: {
        KeySym keysym = 0;
        char buffer[32];
        int count = 0;
        Status status = 0;

        // Use Xutf8LookupString if we have an Input Context,
        // otherwise fall back to the old method.
        if (xic_) {
          count = Xutf8LookupString(xic_, &e.xkey, buffer, sizeof(buffer),
                                    &keysym, &status);
        } else {
          count = XLookupString(&e.xkey, buffer, sizeof(buffer), &keysym, NULL);
        }

        Key translated_key = TranslateX11Key(keysym);
        keys_down_[static_cast<int>(translated_key)] = true;

        if (count > 0) {
          DecodeUTF8ToUTF32(buffer, count, input_characters_);
        }
        break;
      }
      case KeyRelease: {
        KeySym key_sym = XLookupKeysym(&e.xkey, 0);
        Key translated_key = TranslateX11Key(key_sym);
        keys_down_[static_cast<int>(translated_key)] = false;
        break;
      }

      case MotionNotify: {
        mouse_x_ = e.xmotion.x;
        mouse_y_ = e.xmotion.y;
        break;
      }

      case ButtonPress:
        [[fallthrough]];
      case ButtonRelease: {
        // Handle regular click.
        MouseButton button = MouseButton::Unknown;
        if (e.xbutton.button == 1)
          button = MouseButton::Left;
        else if (e.xbutton.button == 2)
          button = MouseButton::Middle;
        else if (e.xbutton.button == 3)
          button = MouseButton::Right;
        if (button != MouseButton::Unknown) {
          mouse_buttons_down_[static_cast<int>(button)] =
              (e.type == ButtonPress);
          mouse_x_ = e.xmotion.x;
          mouse_y_ = e.xmotion.y;
          break;
        }

        // Handle scroll event.
        if (e.xbutton.button == 4)
          mouse_scroll_delta_ += 1.0f;
        else if (e.xbutton.button == 5)
          mouse_scroll_delta_ -= 1.0f;
        break;
      }

      case FocusOut: {
        observer_->LostFocus();
        break;
      }

      case FocusIn: {
        observer_->GainedFocus(false);
        break;
      }

      case ClientMessage: {
        if (static_cast<Atom>(e.xclient.data.l[0]) == wm_delete_window_) {
          observer_->OnWindowDestroyed();
          DestroyWindow();
          should_exit_ = true;
          return;
        }
        break;
      }

      case ConfigureNotify: {
        XConfigureEvent xce = e.xconfigure;
        observer_->OnWindowResized(xce.width, xce.height);
      }
    }
  }
}

void Platform::Exit() {
  should_exit_ = true;
}

void Platform::Vibrate(int duration) {}

void Platform::ShowInterstitialAd() {}

void Platform::ShareFile(const std::string& file_name) {}

void Platform::SetKeepScreenOn(bool keep_screen_on) {}

bool Platform::CreateWindow(int width, int height) {
  // Try to open the local display.
  display_ = XOpenDisplay(NULL);
  if (!display_) {
    LOG(0) << "Can't connect to X server. Try to set the DISPLAY environment "
              "variable (hostname:number.screen_number).";
    return false;
  }

  Window root_window = DefaultRootWindow(display_);

  XVisualInfo* visual_info = GetXVisualInfo(display_);
  if (!visual_info) {
    LOG(0) << "No appropriate visual found.";
    return false;
  }
  LOG(0) << "Visual " << (void*)visual_info->visualid << " selected";

  // Create the main window.
  XSetWindowAttributes window_attributes;
  window_attributes.colormap =
      XCreateColormap(display_, root_window, visual_info->visual, AllocNone);
  window_attributes.event_mask = ExposureMask | KeyPressMask;
  window_ = XCreateWindow(display_, root_window, 0, 0, width, height, 0,
                          visual_info->depth, InputOutput, visual_info->visual,
                          CWColormap | CWEventMask, &window_attributes);
  XMapWindow(display_, window_);
  XStoreName(display_, window_, "kaliber");

  return true;
}

void Platform::DestroyWindow() {
  if (display_) {
    XDestroyWindow(display_, window_);
#if 0  // TODO: Figure out why XCloseDisplay is crashing
    XCloseDisplay(display_);
#endif
    display_ = nullptr;
    window_ = 0;
  }
}

Display* Platform::GetDisplay() {
  return display_;
}

Window Platform::GetWindow() {
  return window_;
}

XVisualInfo* Platform::GetXVisualInfo(Display* display) {
  long visual_mask = VisualScreenMask;
  int num_visuals;
  XVisualInfo visual_info_template = {};
  visual_info_template.screen = DefaultScreen(display);
  return XGetVisualInfo(display, visual_mask, &visual_info_template,
                        &num_visuals);
}

}  // namespace eng

int main(int argc, char** argv) {
  eng::Platform platform;
  eng::KaliberMain(&platform);
  return 0;
}
