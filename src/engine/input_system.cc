#include "engine/input_system.h"

#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/engine.h"
#include "engine/platform/platform.h"

using namespace base;

namespace eng {

void InputSystem::Init(Registry& registry) {
  // Get the global PlayerInput component.
  player_input_ = &registry.GetSingletonComponent<PlayerInput>();
}

void InputSystem::Update(bool mouse_captured, bool keyboard_captured) {
  if (player_input_ == nullptr)
    return;

  // --- 1. Reset "Just Pressed" and "Delta" states ---
  ResetFrameStates();

  // --- 2. Poll Mouse and Keyboard ---
  if (!mouse_captured)
    UpdateMouseState();
  if (!keyboard_captured)
    UpdateKeyboardState();

  // --- 3. Store current states for next frame's comparison ---
  UpdateLastFrameStates();
}

void InputSystem::ResetFrameStates() {
  // Reset all "just pressed" and "delta" values
  player_input_->mouse_x_delta = 0.0f;
  player_input_->mouse_y_delta = 0.0f;
  player_input_->mouse_scroll_delta = 0.0f;
  player_input_->mouse_left_pressed = false;
  player_input_->mouse_right_pressed = false;
  player_input_->mouse_middle_pressed = false;
  player_input_->keys_pressed.fill(false);
}

void InputSystem::UpdateMouseState() {
  // --- 1. Absolute Position ---
  player_input_->mouse_x = Engine::Get().GetPlatform()->GetMouseX();
  player_input_->mouse_y = Engine::Get().GetPlatform()->GetMouseY();

  // Delta
  player_input_->mouse_x_delta = player_input_->mouse_x - last_mouse_x_;
  player_input_->mouse_y_delta = player_input_->mouse_y - last_mouse_y_;

  // --- 3. Scroll Wheel ---
  player_input_->mouse_scroll_delta =
      Engine::Get().GetPlatform()->GetMouseScrollYDelta();

  // --- 4. Mouse Buttons ---
  bool left_down =
      Engine::Get().GetPlatform()->IsMouseButtonDown(MouseButton::Left);
  bool right_down =
      Engine::Get().GetPlatform()->IsMouseButtonDown(MouseButton::Right);
  bool middle_down =
      Engine::Get().GetPlatform()->IsMouseButtonDown(MouseButton::Middle);

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
  for (size_t i = 0; i < static_cast<size_t>(Key::MaxKeys); ++i) {
    bool isDown = Engine::Get().GetPlatform()->IsKeyDown(static_cast<Key>(i));

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
