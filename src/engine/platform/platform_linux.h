#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include <memory>

#include "platform_base.h"

namespace eng {

class PlatformLinux : public PlatformBase {
 public:
  PlatformLinux();
  ~PlatformLinux();

  void Initialize();

  void Update();

  void Exit();

 private:
};

}  // namespace eng

#endif  // PLATFORM_LINUX_H
