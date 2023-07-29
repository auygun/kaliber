#ifndef BASE_MISC_H
#define BASE_MISC_H

#include <cstdint>

#define CRASH *((int*)nullptr) = 0;

namespace base {

inline uint32_t GetHighestBitPos(uint32_t value) {
  uint32_t result = 0;
  if (0xFFFF0000 & value) {
    value &= 0xFFFF0000;
    result += 0x10;
  }
  if (0xFF00FF00 & value) {
    value &= 0xFF00FF00;
    result += 0x8;
  }
  if (0xF0F0F0F0 & value) {
    value &= 0xF0F0F0F0;
    result += 0x04;
  }
  if (0xCCCCCCCC & value) {
    value &= 0xCCCCCCCC;
    result += 0x02;
  }
  if (0xAAAAAAAA & value) {
    result += 0x01;
  }
  return result;
}

inline bool IsPow2(int value) {
  return ((value & (value - 1)) == 0);
}

inline uint32_t RoundUpToPow2(uint32_t val) {
  uint32_t i = 1 << GetHighestBitPos(val);
  return val == i ? val : i << 1;
}

}  // namespace base

#endif  // BASE_MISC_H
