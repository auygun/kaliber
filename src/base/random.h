#ifndef RANDOM_H
#define RANDOM_H

void RandomInit();

float RandomFloat(float offset = 0.0f, float scale = 1.0f);
unsigned RandomUnsigned();
int RandomInt();

#endif  // RANDOM_H
