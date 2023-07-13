#include "engine/audio/audio_mixer.h"

#include <cstring>

#include "base/log.h"
#include "base/task_runner.h"
#include "base/thread_pool.h"
#include "engine/audio/audio_bus.h"
#include "engine/audio/mixer_input.h"

#if defined(__ANDROID__)
#include "engine/audio/audio_sink_oboe.h"
#elif defined(__linux__)
#include "engine/audio/audio_sink_alsa.h"
#endif

using namespace base;

namespace eng {

AudioMixer::AudioMixer()
    : main_thread_task_runner_(TaskRunner::GetThreadLocalTaskRunner()),
#if defined(__ANDROID__)
      audio_sink_{std::make_unique<AudioSinkOboe>(this)} {
#elif defined(__linux__)
      audio_sink_{std::make_unique<AudioSinkAlsa>(this)} {
#endif
  bool res = audio_sink_->Initialize();
  CHECK(res) << "Failed to initialize audio sink.";
}

AudioMixer::~AudioMixer() {
  audio_sink_.reset();
}

void AudioMixer::AddInput(std::shared_ptr<MixerInput> input) {
  DCHECK(audio_enabled_);

  std::lock_guard<std::mutex> scoped_lock(lock_);
  inputs_[0].push_back(input);
}

void AudioMixer::Suspend() {
  audio_sink_->Suspend();
}

void AudioMixer::Resume() {
  audio_sink_->Resume();
}

size_t AudioMixer::GetHardwareSampleRate() {
  return audio_sink_->GetHardwareSampleRate();
}

void AudioMixer::RenderAudio(float* output_buffer, size_t num_frames) {
  {
    std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
    if (scoped_lock)
      inputs_[1].splice(inputs_[1].end(), inputs_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = inputs_[1].begin(); it != inputs_[1].end();) {
    auto audio_bus = (*it)->audio_bus.get();
    unsigned flags = (*it)->flags.load(std::memory_order_relaxed);
    bool marked_for_removal = false;

    if (flags & MixerInput::kStopped) {
      marked_for_removal = true;
    } else {
      const float* src[2] = {audio_bus->GetChannelData(0),
                             audio_bus->GetChannelData(1)};
      if (!src[1])
        src[1] = src[0];  // mono.

      size_t num_samples = audio_bus->samples_per_channel();
      size_t src_index = (*it)->src_index;
      size_t step = (*it)->step.load(std::memory_order_relaxed);
      size_t accumulator = (*it)->accumulator;
      float amplitude = (*it)->amplitude;
      float amplitude_inc =
          (*it)->amplitude_inc.load(std::memory_order_relaxed);
      float max_amplitude =
          (*it)->max_amplitude.load(std::memory_order_relaxed);
      size_t channel_offset = (flags & MixerInput::kSimulateStereo)
                                  ? audio_bus->sample_rate() / 10
                                  : 0;

      DCHECK(num_samples > 0);

      for (size_t i = 0; i < num_frames * kChannelCount;) {
        if (src_index < num_samples) {
          // Mix the 1st channel.
          output_buffer[i++] += src[0][src_index] * amplitude;

          // Mix the 2nd channel. Offset the source index for stereo simulation.
          size_t ind = channel_offset + src_index;
          if (ind < num_samples)
            output_buffer[i++] += src[1][ind] * amplitude;
          else if (flags & MixerInput::kLoop)
            output_buffer[i++] += src[1][ind % num_samples] * amplitude;
          else
            i++;

          // Apply amplitude modification.
          amplitude += amplitude_inc;
          if (amplitude <= 0) {
            marked_for_removal = true;
            break;
          } else if (amplitude > max_amplitude) {
            amplitude = max_amplitude;
          }

          // Advance source index. Apply basic resampling for variations.
          accumulator += step;
          src_index += accumulator / 100;
          accumulator %= 100;
        } else {
          if (audio_bus->EndOfStream()) {
            src_index %= num_samples;
            marked_for_removal = !(flags & MixerInput::kLoop);
            break;
          }

          if (!(*it)->streaming_in_progress.load(std::memory_order_acquire)) {
            src_index %= num_samples;
            (*it)->streaming_in_progress.store(true, std::memory_order_relaxed);

            // Swap buffers and start streaming in background.
            audio_bus->SwapBuffers();
            src[0] = audio_bus->GetChannelData(0);
            src[1] = audio_bus->GetChannelData(1);
            if (!src[1])
              src[1] = src[0];  // mono.
            num_samples = audio_bus->samples_per_channel();

            ThreadPool::Get().PostTask(
                HERE,
                std::bind(&AudioMixer::DoStream, this, *it,
                          flags & MixerInput::kLoop),
                true);
          } else {
            DLOG(0) << "Mixer buffer underrun!";
          }
        }
      }

      (*it)->src_index = src_index;
      (*it)->accumulator = accumulator;
      (*it)->amplitude = amplitude;
    }

    if (marked_for_removal) {
      removed_inputs_.push_back(*it);
      it = inputs_[1].erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = removed_inputs_.begin(); it != removed_inputs_.end();) {
    if (!(*it)->streaming_in_progress.load(std::memory_order_relaxed)) {
      main_thread_task_runner_->PostTask(
          HERE, std::bind(&AudioMixer::EndCallback, this, *it));
      it = removed_inputs_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioMixer::DoStream(std::shared_ptr<MixerInput> input, bool loop) {
  input->audio_bus->Stream(loop);
  input->streaming_in_progress.store(false, std::memory_order_release);
}

void AudioMixer::EndCallback(std::shared_ptr<MixerInput> input) {
  input->streaming = false;

  if (input->pending_audio_bus) {
    input->audio_bus = input->pending_audio_bus;
    input->pending_audio_bus.reset();
  }

  if (input->end_cb)
    input->end_cb();
  if (input->restart_cb) {
    input->restart_cb();
    input->restart_cb = nullptr;
  }
}

}  // namespace eng
