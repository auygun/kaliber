#include "audio.h"

#include <cstring>

#include "../../base/log.h"
#include "../../base/task_runner.h"
#include "../../base/worker.h"
#include "../sound.h"

using namespace base;

namespace eng {

AudioBase::AudioBase()
    : main_thread_task_runner_(TaskRunner::GetThreadLocalTaskRunner()) {}

AudioBase::~AudioBase() = default;

void AudioBase::Play(std::shared_ptr<AudioSample> sample) {
  if (audio_enabled_) {
    std::lock_guard<Spinlock> scoped_lock(lock_);
    samples_[0].push_back(sample);
  } else {
    sample->active = false;
  }
}

void AudioBase::RenderAudio(float* output_buffer, size_t num_frames) {
  {
    std::lock_guard<Spinlock> scoped_lock(lock_);
    samples_[1].splice(samples_[1].end(), samples_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = samples_[1].begin(); it != samples_[1].end();) {
    AudioSample* sample = it->get();

    auto sound = sample->sound.get();
    unsigned flags = sample->flags.load(std::memory_order_relaxed);

    if (flags & AudioSample::kStopped) {
      sample->marked_for_removal = true;
    } else if (!sample->marked_for_removal) {
      const float* src[2] = {sound->GetBuffer(0), sound->GetBuffer(1)};
      if (!src[1])
        src[1] = src[0];  // mono.

      size_t num_samples = sound->GetNumSamples();
      size_t src_index = sample->src_index;
      size_t step = sample->step.load(std::memory_order_relaxed);
      size_t accumulator = sample->accumulator;
      float amplitude = sample->amplitude;
      float amplitude_inc =
          sample->amplitude_inc.load(std::memory_order_relaxed);
      float max_amplitude =
          sample->max_amplitude.load(std::memory_order_relaxed);

      size_t channel_offset =
          (flags & AudioSample::kSimulateStereo) && !sound->is_streaming_sound()
              ? sound->hz() / 10
              : 0;

      DCHECK(num_samples || sound->is_streaming_sound());

      for (size_t i = 0; i < num_frames * kChannelCount;) {
        if (num_samples) {
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
            sample->marked_for_removal = true;
            break;
          } else if (amplitude > max_amplitude) {
            amplitude = max_amplitude;
          }

          // Basic resampling for variations.
          accumulator += step;
          src_index += accumulator / 10;
          accumulator %= 10;
        }

        // Advance source index.
        if (src_index >= num_samples) {
          if (!sound->is_streaming_sound()) {
            src_index %= num_samples;

            if (!(flags & AudioSample::kLoop)) {
              sample->marked_for_removal = true;
              break;
            }
          } else if (!sample->streaming_in_progress.load(
                         std::memory_order_acquire)) {
            if (num_samples)
              src_index %= num_samples;

            if (sound->eof()) {
              sample->marked_for_removal = true;
              break;
            }

            sample->streaming_in_progress.store(true,
                                                std::memory_order_relaxed);

            // Swap buffers and start streaming in background.
            sound->SwapBuffers();
            src[0] = sound->GetBuffer(0);
            src[1] = sound->GetBuffer(1);
            if (!src[1])
              src[1] = src[0];  // mono.
            num_samples = sound->GetNumSamples();

            Worker::Get().EnqueueTask(HERE,
                                      std::bind(&AudioBase::DoStream, this, *it,
                                                flags & AudioSample::kLoop));
          } else if (num_samples) {
            DLOG << "Buffer underrun!";
            src_index %= num_samples;
          }
        }
      }

      sample->src_index = src_index;
      sample->accumulator = accumulator;
      sample->amplitude = amplitude;
    }

    if (sample->marked_for_removal &&
        (!sound->is_streaming_sound() ||
         !sample->streaming_in_progress.load(std::memory_order_relaxed))) {
      sample->marked_for_removal = false;
      main_thread_task_runner_->EnqueueTask(
          HERE, std::bind(&AudioBase::EndCallback, this, *it));
      it = samples_[1].erase(it);
    } else {
      ++it;
    }
  }
}

void AudioBase::DoStream(std::shared_ptr<AudioSample> sample, bool loop) {
  sample->sound->Stream(loop);

  // Memory barrier to ensure all memory writes become visible to the audio
  // thread.
  sample->streaming_in_progress.store(false, std::memory_order_release);
}

void AudioBase::EndCallback(std::shared_ptr<AudioSample> sample) {
  AudioSample* s = sample.get();

  s->active = false;

  if (s->end_cb)
    s->end_cb();
}

}  // namespace eng
