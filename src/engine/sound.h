#ifndef ENGINE_SOUND_H
#define ENGINE_SOUND_H

#include <stdint.h>
#include <memory>
#include <string>

#include "engine/audio/audio_bus.h"

typedef struct mp3dec_ex mp3dec_ex_t;

namespace eng {

// Class for streaming and non-streaming sound assets. Loads and decodes mp3
// files. Resamples the decoded audio to match the system sample rate if
// necessary. Non-streaming sounds Can be shared between multiple audio
// resources and played simultaneously.
class Sound final : public AudioBus {
 public:
  Sound();
  ~Sound() final;

  bool Load(const std::string& file_name, bool stream);

  // AudioBus interface
  void Stream(bool loop) final;
  void SwapBuffers() final;
  void ResetStream() final;
  bool IsEndOfStream() const final { return eos_; }

 private:
  // Buffer holding decoded audio.
  std::unique_ptr<float[]> interleaved_data_;
  size_t samples_per_channel_ = 0;

  std::unique_ptr<char[]> encoded_data_;
  std::unique_ptr<mp3dec_ex_t> mp3_dec_;
  uint64_t read_pos_ = 0;
  bool eos_ = false;

  void StreamInternal(size_t num_samples, bool loop);
};

}  // namespace eng

#endif  // ENGINE_SOUND_H
