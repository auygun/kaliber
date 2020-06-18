#ifndef AUDIO_H
#define AUDIO_H

#if defined(__ANDROID__)
#include "audio_oboe.h"
#elif defined(__linux__)
#include "audio_alsa.h"
#endif

namespace eng {

#if defined(__ANDROID__)
using Audio = AudioOboe;
#elif defined(__linux__)
using Audio = AudioAlsa;
#endif

}  // namespace eng

#endif  // AUDIO_H
