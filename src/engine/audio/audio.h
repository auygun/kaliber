#ifndef AUDIO_H
#define AUDIO_H

#if defined(__ANDROID__)
#include "audio_oboe.h"
#else
#include "audio_null.h"
#endif

namespace eng {

#if defined(__ANDROID__)
using Audio = AudioOboe;
#else
using Audio = AudioNull;
#endif

}  // namespace eng

#endif  // AUDIO_H
