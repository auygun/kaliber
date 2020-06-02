#include "random.h"

#include <limits>

#include "interpolation.h"

namespace base {

Random::Random() {
  std::random_device rd;
  generator_ = std::mt19937(rd());
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
}

Random::Random(unsigned seed) {
  generator_ = std::mt19937(seed);
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
}

Random::~Random() = default;

int Random::Roll(int min, int max) {
  return Lerp(min, max, GetFloat());
}

}  // namespace base
