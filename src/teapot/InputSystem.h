#ifndef TEAPOT_INPUT_SYSTEM_H
#define TEAPOT_INPUT_SYSTEM_H

#include <iostream>
#include <map>

#include "teapot/components.h"
#include "teapot/ecs.h"

namespace eng {

class Platform;

// The one-and-only system responsible for polling hardware.
// This system is stateful as it needs to calculate "just pressed" states and
// mouse deltas. It runs once per frame (before other systems) and fills the
// global PlayerInput resource component.
class InputSystem {
 public:
  InputSystem(Registry* registry, Platform* platform);

  void Update();

 private:
  // Cached Pointers
  Platform* platform_{nullptr};
  PlayerInput* player_input_{nullptr};

  // State tracking (for deltas and "just pressed")
  float last_mouse_x_{0};
  float last_mouse_y_{0};
  bool last_mouse_states_[3] = {false, false, false};  // Left, Right, Middle
  std::array<bool, PlayerInput::kNumKeyCodes> last_key_states_;

  void ResetFrameStates();
  void UpdateMouseState();
  void UpdateKeyboardState();
  void UpdateLastFrameStates();
};

}  // namespace eng

#endif  // TEAPOT_INPUT_SYSTEM_H
