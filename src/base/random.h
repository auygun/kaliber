#ifndef RANDOM_H
#define RANDOM_H

namespace base {

void RandomInit();

float RandomFloat(float offset = 0.0f, float scale = 1.0f);
unsigned RandomUnsigned();
int RandomInt();

} // namespace base

#endif  // RANDOM_H
