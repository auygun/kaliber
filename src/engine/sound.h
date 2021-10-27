#ifndef ENGINE_SOUND_H
#define ENGINE_SOUND_H

#include <stdint.h>
#include <memory>
#include <string>

typedef struct mp3dec_ex mp3dec_ex_t;

namespace base {
class SincResampler;
}  // namespace base

namespace eng {

// Class for streaming and non-streaming sound assets. Loads and decodes mp3
// files. Resamples the decoded audio to match the system sample rate if
// necessary. Non-streaming sounds Can be shared between multiple audio
// resources and played simultaneously.
class Sound {
 public:
  Sound();
  ~Sound();

  bool Load(const std::string& file_name, bool stream);

  bool Stream(bool loop);

  void SwapBuffers();

  void ResetStream();

  float* GetBuffer(int channel) const;

  size_t GetNumSamples() const { return num_samples_front_; }

  size_t num_channels() const { return num_channels_; }
  int sample_rate() const { return sample_rate_; }

  bool is_streaming_sound() const { return is_streaming_sound_; }

  bool eof() const { return eof_; }

 private:
  // Buffer holding decoded audio.
  std::unique_ptr<float[]> back_buffer_[2];
  std::unique_ptr<float[]> front_buffer_[2];

  size_t num_samples_back_ = 0;
  size_t num_samples_front_ = 0;
  size_t max_samples_ = 0;

  size_t cur_sample_front_ = 0;
  size_t cur_sample_back_ = 0;

  size_t num_channels_ = 0;
  int sample_rate_ = 0;

  int hw_sample_rate_ = 0;

  std::unique_ptr<char[]> encoded_data_;

  std::unique_ptr<mp3dec_ex_t> mp3_dec_;

  std::unique_ptr<base::SincResampler> resampler_[2];

  bool eof_ = false;

  bool is_streaming_sound_ = false;

  bool StreamInternal(size_t num_samples, bool loop);

  void Preprocess(std::unique_ptr<float[]> input_buffer,
                  size_t samples_per_channel);
};

}  // namespace eng

#endif  // ENGINE_SOUND_H
