#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(__ANDROID__)
#include "platform_android.h"
#elif defined(__linux__)
#include "platform_linux.h"
#endif

namespace eng {

#if defined(__ANDROID__)
using Platform = PlatformAndroid;
#elif defined(__linux__)
using Platform = PlatformLinux;
#endif

}  // namespace eng

#endif  // PLATFORM_H
