#ifndef FRAME_CONTROLLER_H
#define FRAME_CONTROLLER_H

#include "../base/vecmath.h"
#include <cstdlib>

namespace engine {

class FrameController {
 public:
  virtual ~FrameController() = default;

  virtual size_t GetNumFrames() = 0;
  virtual size_t GetCurrentFrame() = 0;
  virtual void SetCurrentFrame(size_t frame) = 0;
};

}  // namespace engine

#endif  // FRAME_CONTROLLER_H
