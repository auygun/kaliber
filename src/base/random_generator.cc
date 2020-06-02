#include "random_generator.h"

#include <limits>

#include "interpolation.h"

namespace base {

RandomGenerator::RandomGenerator() {
  std::random_device rd;
  generator_ = std::mt19937(rd());
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
}

RandomGenerator::RandomGenerator(unsigned seed) {
  generator_ = std::mt19937(seed);
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
}

RandomGenerator::~RandomGenerator() = default;

int RandomGenerator::Roll(int min, int max) {
  return Lerp(min, max, GetFloat());
}

}  // namespace base
