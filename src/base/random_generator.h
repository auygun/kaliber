#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <random>

namespace base {

// Generates random doubles between 0 and 1 and integers.
class RandomGenerator {
 public:
  RandomGenerator();
  RandomGenerator(unsigned seed);
  ~RandomGenerator();

  double GetDouble() { return real_distribution_(generator_); }
  int GetInt() { return int_distribution_(generator_); }
  unsigned GetUInt() { return uint_distribution_(generator_); }

 private:
  std::mt19937 generator_;
  std::uniform_real_distribution<double> real_distribution_;
  std::uniform_int_distribution<int> int_distribution_;
  std::uniform_int_distribution<unsigned> uint_distribution_;

  void Initialize(unsigned seed);
};

}  // namespace base

#endif  // RANDOM_GENERATOR_H
