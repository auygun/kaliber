#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <cassert>
#include "../base/vecmath.h"

namespace eng {

class InputEvent {
 public:
  enum Type {
    kInvalid,
    kTap,
    kDoubleTap,
    kDragStart,
    kDrag,
    kDragEnd,
    kDragCancel,
    kPinchStart,
    kPinch,
    kNavigateBack,
    kKeyPress,
    kType_Max  // Not a type.
  };

  InputEvent(Type type) : type_(type) {}
  InputEvent(Type type, const base::Vector2& vec1)
      : type_(type), vec_{vec1, {0, 0}} {}
  InputEvent(Type type, const base::Vector2& vec1, const base::Vector2& vec2)
      : type_(type), vec_{vec1, vec2} {}
  InputEvent(Type type, char key) : type_(type), key_(key) {}
  ~InputEvent() = default;

  Type GetType() { return type_; }
  base::Vector2 GetVector(size_t i) {
    assert(i < 2);
    return vec_[i];
  }
  char GetKeyPress() { return key_; }

 private:
  Type type_ = kInvalid;
  base::Vector2 vec_[2] = {{0, 0}, {0, 0}};
  char key_ = 0;
};

}  // namespace eng

#endif  // INPUT_EVENT_H
