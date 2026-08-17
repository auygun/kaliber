#ifndef ENGINE_INPUT_SYSTEM_H
#define ENGINE_INPUT_SYSTEM_H

#include <array>

#include "engine/input_codes.h"

namespace eng {

class Registry;
struct PlayerInput;

// The one-and-only system responsible for polling hardware.
// This system is stateful as it needs to calculate "just pressed" states and
// mouse deltas. It runs once per frame (before other systems) and fills the
// global PlayerInput resource component.
class InputSystem {
 public:
  InputSystem() = default;
  ~InputSystem() = default;

  void Init(Registry& registry);

  void Update(bool mouse_captured, bool keyboard_captured);

 private:
  // Cached Pointers
  PlayerInput* player_input_{nullptr};

  // State tracking (for deltas and "just pressed")
  float last_mouse_x_{0};
  float last_mouse_y_{0};
  std::array<bool, static_cast<int>(MouseButton::MaxButtons)>
      last_mouse_states_{};
  std::array<bool, static_cast<int>(Key::MaxKeys)> last_key_states_;

  void ResetFrameStates();
  void UpdateMouseState();
  void UpdateKeyboardState();
  void UpdateLastFrameStates();
};

}  // namespace eng

#endif  // ENGINE_INPUT_SYSTEM_H
