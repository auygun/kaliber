#ifndef ENGINE_AUDIO_AUDIO_BUS_H
#define ENGINE_AUDIO_AUDIO_BUS_H

#include <memory>

namespace eng {

class SincResampler;

// Represents a sequence of audio samples for each channels. The data layout is
// planar as opposed to interleaved. The memory for the data is allocated and
// owned by the AudioBus. Max two channels are supported. An AudioBus with one
// channel is mono, with two channels is stereo.
class AudioBus {
 public:
  AudioBus();
  virtual ~AudioBus();

  virtual void Stream(bool loop) = 0;
  virtual void SwapBuffers() = 0;
  virtual void ResetStream() = 0;
  virtual bool EndOfStream() const = 0;

  float* GetChannelData(int channel) const {
    return channel_data_[channel].get();
  }

  size_t samples_per_channel() const { return samples_per_channel_; }
  int sample_rate() const { return sample_rate_; }
  int num_channels() const { return num_channels_; }

 protected:
  void SetAudioConfig(size_t num_channels, size_t sample_rate);

  // Overwrites the sample values stored in this AudioBus instance with values
  // from a given interleaved source_buffer. The expected layout of the
  // source_buffer is [ch0, ch1, ch0, ch1, ...]. A sample-rate conversion to the
  // system sample-rate will be made if it doesn't match.
  void FromInterleaved(std::unique_ptr<float[]> source_buffer,
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
