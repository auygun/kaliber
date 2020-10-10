#include "random.h"

#include <limits>

#include "interpolation.h"
#include "log.h"

namespace base {

Random::Random() {
  std::random_device rd;
  seed_ = rd();
  DLOG << "Random seed: " << seed_;
  Initialize();
}

Random::Random(unsigned seed) {
  seed_ = seed;
  Initialize();
}

Random::~Random() = default;

float Random::GetFloat() {
  return real_distribution_(generator_);
}

int Random::Roll(int sides) {
  return Lerp(1, sides, GetFloat());
}

void Random::Initialize() {
  generator_ = std::mt19937(seed_);
  real_distribution_ = std::uniform_real_distribution<float>(0, 1);
}

}  // namespace base
