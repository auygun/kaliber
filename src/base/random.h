#ifndef BASE_RANDOM_H
#define BASE_RANDOM_H

#include <random>

#include "base/interpolation.h"

namespace base {

template <typename T>
class Random {
 public:
  Random() {
    std::random_device rd;
    seed_ = rd();
    Initialize();
  }

  Random(unsigned seed) : seed_(seed) { Initialize(); }

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

  void Initialize() {
    generator_ = std::mt19937(seed_);
    real_distribution_ = std::uniform_real_distribution<T>(0, 1);
  }
};

using Randomf = Random<float>;

}  // namespace base

#endif  // BASE_RANDOM_H
