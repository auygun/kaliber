#ifndef AUDIO_NULL_H
#define AUDIO_NULL_H

namespace eng {

class AudioNull {
 public:
  AudioNull() = default;
  ~AudioNull() = default;

  bool Initialize() { return true; }

  void Shutdown() {}
};

}  // namespace eng

#endif  // AUDIO_NULL_H
