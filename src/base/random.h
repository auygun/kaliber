#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <random>

namespace base {

class Random {
 public:
  Random();
  Random(unsigned seed);
  ~Random();

  // Returns a random float between 0 and 1.
  float GetFloat();

  // Roll dice with the given number of sides.
  int Roll(int sides);

  unsigned seed() const { return seed_; }

 private:
  unsigned seed_ = 0;
  std::mt19937 generator_;
  std::uniform_real_distribution<float> real_distribution_;

  void Initialize();
};

}  // namespace base

#endif  // RANDOM_GENERATOR_H
