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
  real_distribution_ = std::uniform_real_distribution<double>(0, 1);
  int_distribution_ =
      std::uniform_int_distribution<int>(std::numeric_limits<unsigned>::min(),
                                         std::numeric_limits<unsigned>::max());
  uint_distribution_ = std::uniform_int_distribution<unsigned>(
      0, std::numeric_limits<unsigned>::max());
}

}  // namespace base
