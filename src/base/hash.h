#ifndef BASE_HASH_H
#define BASE_HASH_H

#include <stddef.h>
#include <string>

namespace base {

// Compile-time string hashing function.
template <size_t N>
constexpr inline size_t Hash(const char (&str)[N], size_t Len = N - 1) {
  size_t hash_value = 0;
  for (int i = 0; str[i] != '\0'; ++i)
    hash_value = str[i] + 31 * hash_value;
  return hash_value;
}

// The same hashing function for run-time.
inline size_t Hash(const std::string& str) {
  size_t hash_value = 0;
  for (std::string::value_type c : str)
    hash_value = c + 31 * hash_value;
  return hash_value;
}

}  // namespace base

#endif  // BASE_HASH_H
