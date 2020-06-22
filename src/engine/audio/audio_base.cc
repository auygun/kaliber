#include "audio_base.h"

#include <cstring>

#include "../../base/log.h"
#include "../sound.h"

using namespace base;

namespace eng {

AudioBase::AudioBase() = default;

AudioBase::~AudioBase() = default;

void AudioBase::Play(std::shared_ptr<void> impl_data,
                     std::shared_ptr<const Sound> sound,
                     float amplitude,
                     bool reset_pos) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (sample->active)
    return;

  if (reset_pos) {
    sample->src_index = 0;
    sample->accumulator = 0;
  }
  sample->flags &= ~kStopped;
  sample->sound = sound;
  sample->amplitude = amplitude;
  sample->active = true;

  std::unique_lock<std::mutex> scoped_lock(mutex_);
  samples_[0].push_back(sample);
}

void AudioBase::Stop(std::shared_ptr<void> impl_data) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (!sample->active)
    return;

  sample->flags |= kStopped;
}

void AudioBase::SetLoop(std::shared_ptr<void> impl_data, bool loop) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (loop)
    sample->flags |= kLoop;
  else
    sample->flags &= ~kLoop;
}

void AudioBase::SetSimulateStereo(std::shared_ptr<void> impl_data,
                                  bool simulate) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (simulate)
    sample->flags |= kSimulateStereo;
  else
    sample->flags &= ~kSimulateStereo;
}

void AudioBase::SetResampleStep(std::shared_ptr<void> impl_data, size_t step) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->step = step + 10;
}

void AudioBase::SetMaxAmplitude(std::shared_ptr<void> impl_data,
                                float max_amplitude) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->max_amplitude = max_amplitude;
}

void AudioBase::SetAmplitudeInc(std::shared_ptr<void> impl_data,
                                float amplitude_inc) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->amplitude_inc = amplitude_inc;
}

void AudioBase::SetEndCallback(std::shared_ptr<void> impl_data,
                               base::Closure cb) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->end_cb = cb;
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
    Sample* sample = it->get();

    unsigned flags = sample->flags;
    bool remove = false;

    if (flags & kStopped) {
      remove = true;
    } else {
      const float* src[2] = {sample->sound->GetBuffer(0),
                             sample->sound->GetBuffer(1)};
      if (!src[1])
        src[1] = src[0];
      size_t num_samples = sample->sound->num_samples();
      size_t num_channels = sample->sound->num_channels();
      size_t src_index = sample->src_index;
      size_t step = sample->step;
      size_t accumulator = sample->accumulator;
      float amplitude = sample->amplitude;
      float amplitude_inc = sample->amplitude_inc;
      float max_amplitude = sample->max_amplitude;

      size_t channel_offset = (flags & kSimulateStereo) && num_channels == 1
                                  ? sample->sound->hz() / 10
                                  : 0;

      for (size_t i = 0; i < num_frames * kChannelCount;) {
        // Mix the 1st channel.
        output_buffer[i++] += src[0][src_index] * amplitude;

        // Mix the 2nd channel. Offset the source index for stereo simulation.
        size_t ind = channel_offset + src_index;
        if (ind < num_samples)
          output_buffer[i++] += src[1][ind] * amplitude;
        else if (flags & kLoop)
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
        if (flags & kLoop) {
          src_index %= num_samples;
        } else if (src_index >= num_samples) {
          remove = true;
          break;
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
