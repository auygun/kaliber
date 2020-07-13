#ifndef PLATFORM_FORWARD_H
#define PLATFORM_FORWARD_H

namespace eng {

#if defined(__ANDROID__)
class PlatformAndroid;
using Platform = PlatformAndroid;
#elif defined(__linux__)
class PlatformLinux;
using Platform = PlatformLinux;
#endif

}  // namespace eng

#endif  // PLATFORM_FORWARD_H
