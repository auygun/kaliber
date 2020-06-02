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
  float GetFloat() { return real_distribution_(generator_); }

  // Returns a random int between min and max.
  int Roll(int min, int max);

 private:
  std::mt19937 generator_;
  std::uniform_real_distribution<float> real_distribution_;
};

}  // namespace base

#endif  // RANDOM_GENERATOR_H
