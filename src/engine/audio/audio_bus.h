#ifndef ENGINE_AUDIO_AUDIO_BUS_H
#define ENGINE_AUDIO_AUDIO_BUS_H

#include <memory>

namespace eng {

class SincResampler;

class AudioBus {
 public:
  AudioBus();
  virtual ~AudioBus();

  virtual void Stream(bool loop) = 0;
  virtual void SwapBuffers() = 0;
  virtual void ResetStream() = 0;
  virtual bool IsEndOfStream() const = 0;

  float* GetChannelData(int channel) const {
    return channel_data_[channel].get();
  }

  size_t samples_per_channel() const { return samples_per_channel_; }
  int sample_rate() const { return sample_rate_; }
  int num_channels() const { return num_channels_; }

 protected:
  void SetAudioConfig(size_t num_channels, size_t sample_rate);

  void FromInterleaved(std::unique_ptr<float[]> input_buffer,
                       size_t samples_per_channel);

 private:
  std::unique_ptr<float[]> channel_data_[2];
  size_t samples_per_channel_ = 0;
  size_t sample_rate_ = 0;
  size_t num_channels_ = 0;

  std::unique_ptr<SincResampler> resampler_[2];
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_BUS_H
