#include "audio_base.h"

#include <cstring>

#include "../../base/log.h"
#include "../sound.h"

using namespace base;

namespace eng {

AudioBase::AudioBase() = default;

AudioBase::~AudioBase() {
  worker_.Join();
}

void AudioBase::Play(std::shared_ptr<AudioSample> sample) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  samples_[0].push_back(sample);
}

void AudioBase::Update() {
  task_runner_.Run();
}

void AudioBase::RenderAudio(float* output_buffer, size_t num_frames) {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    samples_[1].splice(samples_[1].end(), samples_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = samples_[1].begin(); it != samples_[1].end();) {
    AudioSample* sample = it->get();

    unsigned flags = sample->flags;
    bool remove = false;

    if (flags & AudioSample::kStopped) {
      remove = true;
    } else {
      auto sound = sample->sound.get();

      const float* src[2] = {const_cast<const Sound*>(sound)->GetBuffer(0),
                             const_cast<const Sound*>(sound)->GetBuffer(1)};
      if (!src[1])
        src[1] = src[0];  // mono.

      size_t num_samples = sound->GetNumSamples();
      size_t num_channels = sound->num_channels();
      size_t src_index = sample->src_index;
      size_t step = sample->step;
      size_t accumulator = sample->accumulator;
      float amplitude = sample->amplitude;
      float amplitude_inc = sample->amplitude_inc;
      float max_amplitude = sample->max_amplitude;

      size_t channel_offset =
          (flags & AudioSample::kSimulateStereo) && num_channels == 1
              ? sound->hz() / 10
              : 0;

      for (size_t i = 0; i < num_frames * kChannelCount;) {
        // Mix the 1st channel.
        output_buffer[i++] += src[0][src_index] * amplitude;

        // Mix the 2nd channel. Offset the source index for stereo simulation.
        size_t ind = channel_offset + src_index;
        if (ind < num_samples)
          output_buffer[i++] += src[1][ind] * amplitude;
        else if (flags & AudioSample::kLoop)
          output_buffer[i++] += src[1][ind % num_samples] * amplitude;
        else
          i++;

        // Apply amplitude modification.
        amplitude += amplitude_inc;
        if (amplitude <= 0) {
          remove = true;
          break;
        } else if (amplitude > max_amplitude) {
          amplitude = max_amplitude;
        }

        // Basic resampling for variations.
        accumulator += step;
        src_index += accumulator / 10;
        accumulator %= 10;

        // Advance source index.
        if (src_index >= num_samples) {
          if (!sound->is_streaming_sound()) {
            if (flags & AudioSample::kLoop) {
              src_index %= num_samples;
            } else {
              remove = true;
              break;
            }
          } else if (!sound->IsStreamingInProgress()) {
            if (sound->eof()) {
              remove = true;
              break;
            }

            src_index = 0;

            // Swap buffers and start streaming in background.
            sound->SwapBuffers();
            src[0] = const_cast<const Sound*>(sound)->GetBuffer(0);
            src[1] = const_cast<const Sound*>(sound)->GetBuffer(1);

            worker_.Enqueue(std::bind(&Sound::Stream, sample->sound,
                                      flags & AudioSample::kLoop));
          } else {
            LOG << "Buffer underrun!";
            src_index = 0;
          }
        }
      }

      sample->src_index = src_index;
      sample->accumulator = accumulator;
      sample->amplitude = amplitude;
    }

    if (remove) {
      task_runner_.Enqueue(sample->end_cb);
      sample->active = false;
      it = samples_[1].erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace eng
