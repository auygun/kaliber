#ifndef HASH_H
#define HASH_H

#include <stddef.h>

#define HHASH(x) base::HornerHash(31, x)

namespace base {

// Compile time string hashing function.
template <size_t N>
constexpr inline size_t HornerHash(size_t prime,
                                   const char (&str)[N],
                                   size_t Len = N - 1) {
  return (Len <= 1) ? str[0]
                    : (prime * HornerHash(prime, str, Len - 1) + str[Len - 1]);
}

}  // namespace base

#endif  // HASH_H
