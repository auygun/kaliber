#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "../base/vecmath.h"
#include <cassert>

namespace engine {

class InputEvent {
 public:
  enum Type {
    kInvalid,
    kDoubleTap,
    kDragStart,
    kDrag,
    kDragEnd,
    kPinchStart,
    kPinch,
    kType_Max // Not a type.
  };

  InputEvent(Type type, const Vector2& vec1, const Vector2& vec2)
      : type_(type), vec_{vec1, vec2} {}
  ~InputEvent() = default;

  Type GetEventType() { return type_; }
  Vector2 GetEventVector(size_t i) { assert(i < 2); return vec_[i]; }

 private:
  Type type_;
  Vector2 vec_[2];
};

}  // namespace engine

#endif  // INPUT_EVENT_H
