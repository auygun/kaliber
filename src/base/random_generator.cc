#include "random_generator.h"
#include <limits>

namespace base {

RandomGenerator::RandomGenerator() {
  std::random_device rd;
  Initialize(rd());
}

RandomGenerator::RandomGenerator(unsigned seed) {
  Initialize(seed);
}

RandomGenerator::~RandomGenerator() = default;

void RandomGenerator::Initialize(unsigned seed) {
  generator_ = std::mt19937(seed);
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
  int_distribution_ =
      std::uniform_int_distribution<int>(0, std::numeric_limits<int>::max());
}

}  // namespace base
