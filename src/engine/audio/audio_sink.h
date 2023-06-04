#ifndef ENGINE_AUDIO_AUDIO_SINK_H
#define ENGINE_AUDIO_AUDIO_SINK_H

namespace eng {

// Models an audio sink sending mixed audio to the audio driver. Audio data from
// the mixer source is delivered on a pull model using Delegate.
class AudioSink {
 public:
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual int GetChannelCount() = 0;

    virtual void RenderAudio(float* output_buffer, size_t num_frames) = 0;
  };

  AudioSink() = default;
  virtual ~AudioSink() = default;

  virtual bool Initialize() = 0;

  virtual void Suspend() = 0;
  virtual void Resume() = 0;

  virtual size_t GetHardwareSampleRate() = 0;

 private:
  AudioSink(const AudioSink&) = delete;
  AudioSink& operator=(const AudioSink&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_SINK_H
