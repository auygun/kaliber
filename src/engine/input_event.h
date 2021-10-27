#ifndef ENGINE_INPUT_EVENT_H
#define ENGINE_INPUT_EVENT_H

#include "base/log.h"
#include "base/vecmath.h"

namespace eng {

class InputEvent {
 public:
  enum Type {
    kInvalid,
    kDragStart,
    kDrag,
    kDragEnd,
    kDragCancel,
    kNavigateBack,
    kKeyPress,
    kType_Max  // Not a type.
  };

  InputEvent(Type type, size_t pointer_id)
      : type_(type), pointer_id_(pointer_id) {}
  InputEvent(Type type, size_t pointer_id, const base::Vector2f& vec)
      : type_(type), pointer_id_(pointer_id), vec_(vec) {}
  InputEvent(Type type) : type_(type) {}
  InputEvent(Type type, char key) : type_(type), key_(key) {}
  ~InputEvent() = default;

  Type GetType() const { return type_; }

  size_t GetPointerId() const { return pointer_id_; }

  base::Vector2f GetVector() const { return vec_; }

  char GetKeyPress() const { return key_; }

 private:
  Type type_ = kInvalid;
  size_t pointer_id_ = 0;
  base::Vector2f vec_ = {0, 0};
  char key_ = 0;
};

}  // namespace eng

#endif  // ENGINE_INPUT_EVENT_H
