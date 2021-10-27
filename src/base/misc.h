#ifndef BASE_MISC_H
#define BASE_MISC_H

#define CRASH *((int*)nullptr) = 0;

namespace base {

// ToDo: x86 has the bsr instruction.
inline int GetHighestBitPos(int value) {
  return (0xFFFF0000 & value ? value &= 0xFFFF0000, 1 : 0) * 0x10 +
         (0xFF00FF00 & value ? value &= 0xFF00FF00, 1 : 0) * 0x08 +
         (0xF0F0F0F0 & value ? value &= 0xF0F0F0F0, 1 : 0) * 0x04 +
         (0xCCCCCCCC & value ? value &= 0xCCCCCCCC, 1 : 0) * 0x02 +
         (0xAAAAAAAA & value ? 1 : 0) * 0x01;
}

// Get the highest set bit in an integer number
inline int GetHighestBit(int value) {
  return 0x1 << GetHighestBitPos(value);
}

// Check if the given integer is a power of two, ie if only one bit is set.
inline bool IsPow2(int value) {
  return GetHighestBit(value) == value;
}

inline int RoundUpToPow2(int val) {
  int i = GetHighestBit(val);
  return val == i ? val : i << 1;
}

}  // namespace base

#endif  // BASE_MISC_H
