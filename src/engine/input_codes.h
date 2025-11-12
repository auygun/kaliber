#ifndef ENGINE_INPUT_CODES_H
#define ENGINE_INPUT_CODES_H

namespace eng {

// Defines platform-agnostic key codes.
enum class Key {
    Unknown,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    Escape,
    Space,
    Enter,
    Tab,
    Backspace,
    Up,
    Down,
    Left,
    Right,
    PageUp,
    PageDown,
    Home,
    End,
    Insert,
    Delete,
    ShiftLeft,
    ShiftRight,
    ControlLeft,
    ControlRight,
    AltLeft,
    AltRight,
    // Add more keys as needed
    MaxKeys,
};

/**
 * @brief Defines platform-agnostic mouse buttons.
 */
enum class MouseButton {
    Unknown,
    Left,
    Right,
    Middle,
    MaxButtons,
};

}  // namespace eng

#endif  // ENGINE_INPUT_CODES_H
