#ifndef BASE_RANDOM_H
#define BASE_RANDOM_H

#include <random>

#include "base/interpolation.h"

namespace base {

template <typename T>
class Random {
 public:
  Random()
      : seed_(std::random_device{}()),
        generator_(seed_),
        real_distribution_(0, 1) {}

  Random(unsigned seed)
      : seed_(seed),
        generator_(seed),
        real_distribution_(0, 1) {}

  ~Random() = default;

  // Returns a random between 0 and 1.
  T Rand() { return real_distribution_(generator_); }

  // Roll dice with the given number of sides.
  int Roll(int sides) { return Lerp(1, sides + 1, Rand()); }

  unsigned seed() const { return seed_; }

 private:
  unsigned seed_ = 0;
  std::mt19937 generator_;
  std::uniform_real_distribution<T> real_distribution_;
};

using Randomf = Random<float>;

}  // namespace base

#endif  // BASE_RANDOM_H
