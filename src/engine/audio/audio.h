#ifndef ENGINE_AUDIO_AUDIO_H
#define ENGINE_AUDIO_AUDIO_H

#if defined(__ANDROID__)
#include "engine/audio/audio_oboe.h"
#elif defined(__linux__)
#include "engine/audio/audio_alsa.h"
#endif

namespace eng {

#if defined(__ANDROID__)
using Audio = AudioOboe;
#elif defined(__linux__)
using Audio = AudioAlsa;
#endif

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_H
