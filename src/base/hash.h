#ifndef BASE_HASH_H
#define BASE_HASH_H

#include <stddef.h>
#include <string>

namespace base {

template <size_t N>
constexpr inline size_t KR2Hash(const char (&str)[N], size_t Len = N - 1) {
  size_t hash_value = 0;
  for (int i = 0; str[i] != '\0'; ++i)
    hash_value = str[i] + 31 * hash_value;
  return hash_value;
}

inline size_t KR2Hash(const std::string& str) {
  size_t hash_value = 0;
  for (std::string::value_type c : str)
    hash_value = c + 31 * hash_value;
  return hash_value;
}

inline uint32_t HashVec32(const std::vector<uint32_t>& vec) {
  uint32_t seed = vec.size();
  for (auto x : vec) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

inline uint64_t HashVec64(const std::vector<uint64_t>& vec) {
  uint64_t seed = vec.size();
  for (auto x : vec) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    seed ^= x + UINT64_C(0x9e3779b97f4a7c15) + (seed << 12) + (seed >> 4);
  }
  return seed;
}

}  // namespace base

#endif  // BASE_HASH_H
