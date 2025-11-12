#include "teapot/InputSystem.h"

namespace eng {

InputSystem::InputSystem(Registry* registry, Platform* platform)
    : platform_(platform), last_mouse_x_(0.0f), last_mouse_y_(0.0f) {
  // Get the global PlayerInput component.
  player_input_ = &registry->GetSingletonComponent<PlayerInput>();
}

void InputSystem::Update() {
  if (player_input_ == nullptr)
    return;

  // --- 1. Reset "Just Pressed" and "Delta" states ---
  ResetFrameStates();

  // --- 2. Poll Mouse and Keyboard ---
  UpdateMouseState();
  UpdateKeyboardState();

  // --- 3. Store current states for next frame's comparison ---
  UpdateLastFrameStates();
}

void InputSystem::ResetFrameStates() {
  // Reset all "just pressed" and "delta" values
  player_input_->mouse_scroll_delta = 0.0f;
  player_input_->mouse_left_pressed = false;
  player_input_->mouse_right_pressed = false;
  player_input_->mouse_middle_pressed = false;
  player_input_->keys_pressed.fill(false);
}

void InputSystem::UpdateMouseState() {
  // --- 1. Absolute Position ---
  player_input_->mouse_x = platform_->GetMouseX();
  player_input_->mouse_y = platform_->GetMouseY();

  // --- 3. Scroll Wheel ---
  player_input_->mouse_scroll_delta = platform_->GetMouseScrollDelta();

  // --- 4. Mouse Buttons ---
  bool left_down = platform_->IsMouseButtonDown(MouseButton::LEFT);
  bool right_down = platform_->IsMouseButtonDown(MouseButton::RIGHT);
  bool middle_down = platform_->IsMouseButtonDown(MouseButton::MIDDLE);

  // "Held" states
  player_input_->mouse_left_held = left_down;
  player_input_->mouse_right_held = right_down;
  player_input_->mouse_middle_held = middle_down;

  // "Just Pressed" states
  player_input_->mouse_left_pressed = left_down && !last_mouse_states_[0];
  player_input_->mouse_right_pressed = right_down && !last_mouse_states_[1];
  player_input_->mouse_middle_pressed = middle_down && !last_mouse_states_[2];
}

void InputSystem::UpdateKeyboardState() {
  // Iterate over all possible key codes supported by PlayerInput (0 to 255)
  for (size_t i = 0; i < PlayerInput::kNumKeyCodes; ++i) {
    // Cast index to KeyCode.
    // In a real engine, you would likely map the index to a specific OS
    // scancode if your KeyCode enum values aren't sequential integers.
    KeyCode key = static_cast<KeyCode>(i);

    bool isDown = platform_->IsKeyDown(key);

    // Fill "Held" state
    player_input_->keys_held[i] = isDown;

    // Fill "Just Pressed" state
    player_input_->keys_pressed[i] = isDown && !last_key_states_[i];
  }
}

void InputSystem::UpdateLastFrameStates() {
  last_mouse_x_ = player_input_->mouse_x;
  last_mouse_y_ = player_input_->mouse_y;
  last_mouse_states_[0] = player_input_->mouse_left_held;
  last_mouse_states_[1] = player_input_->mouse_right_held;
  last_mouse_states_[2] = player_input_->mouse_middle_held;
  last_key_states_ = player_input_->keys_held;
}

}  // namespace eng

#endif  // TEAPOT_INPUT_SYSTEM_H
