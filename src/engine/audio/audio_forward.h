#ifndef AUDIO_FORWARD_H
#define AUDIO_FORWARD_H

namespace eng {

#if defined(__ANDROID__)
class AudioOboe;
using Audio = AudioOboe;
#else
class AudioNull;
using Audio = AudioNull;
#endif

}  // namespace eng

#endif  // AUDIO_FORWARD_H
