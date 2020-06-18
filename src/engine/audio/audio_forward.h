#ifndef AUDIO_FORWARD_H
#define AUDIO_FORWARD_H

namespace eng {

#if defined(__ANDROID__)
class AudioOboe;
using Audio = AudioOboe;
#elif defined(__linux__)
class AudioAlsa;
using Audio = AudioAlsa;
#else
class AudioNull;
using Audio = AudioNull;
#endif

}  // namespace eng

#endif  // AUDIO_FORWARD_H
