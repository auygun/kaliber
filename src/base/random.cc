#include "random.h"
#include <stdlib.h>

namespace base {

void RandomInit() {
  srand(42);
}

float RandomFloat(float offset, float scale) {
  float r = rand() / (float)RAND_MAX;
  return r * scale - offset;
}

unsigned RandomUnsigned() {
  return (unsigned)rand();
}

int RandomInt() {
  return (int)rand();
}

} // namespace base
