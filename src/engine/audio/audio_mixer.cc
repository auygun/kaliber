#include "engine/audio/audio_mixer.h"

#include <cstring>

#include "base/log.h"
#include "base/task_runner.h"
#include "base/thread_pool.h"
#include "engine/audio/audio_bus.h"

#if defined(__ANDROID__)
#include "engine/audio/audio_sink_oboe.h"
#elif defined(__linux__)
#include "engine/audio/audio_sink_alsa.h"
#endif

using namespace base;

namespace eng {

AudioMixer::Resource::~Resource() {
  DLOG(1) << "Destroying audio resource: " << uintptr_t(this);
}

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

std::shared_ptr<void> AudioMixer::CreateResource() {
  auto resource = std::make_shared<Resource>();
  DLOG(1) << "Audio resource created: " << uintptr_t(resource.get());
  return resource;
}

void AudioMixer::Play(std::shared_ptr<void> resource,
                      std::shared_ptr<AudioBus> audio_bus,
                      float amplitude,
                      bool reset_pos) {
  if (!audio_enabled_)
    return;

  auto resource_ptr = std::static_pointer_cast<Resource>(resource);

  if (resource_ptr->active) {
    if (reset_pos)
      resource_ptr->flags.fetch_or(kStopped, std::memory_order_relaxed);

    if (resource_ptr->flags.load(std::memory_order_relaxed) & kStopped)
      resource_ptr->restart_cb = [&, resource, audio_bus, amplitude,
                                  reset_pos]() -> void {
        Play(resource, audio_bus, amplitude, reset_pos);
      };

    return;
  }

  if (reset_pos) {
    resource_ptr->src_index = 0;
    resource_ptr->accumulator = 0;
    audio_bus->ResetStream();
  } else if (resource_ptr->src_index >= audio_bus->samples_per_channel()) {
    return;
  }

  resource_ptr->active = true;
  resource_ptr->flags.fetch_and(~kStopped, std::memory_order_relaxed);
  resource_ptr->audio_bus = audio_bus;
  if (amplitude >= 0)
    resource_ptr->amplitude = amplitude;

  std::lock_guard<std::mutex> scoped_lock(lock_);
  play_list_[0].push_back(resource_ptr);
}

void AudioMixer::Stop(std::shared_ptr<void> resource) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  if (resource_ptr->active) {
    resource_ptr->restart_cb = nullptr;
    resource_ptr->flags.fetch_or(kStopped, std::memory_order_relaxed);
  }
}

void AudioMixer::SetLoop(std::shared_ptr<void> resource, bool loop) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  if (loop)
    resource_ptr->flags.fetch_or(kLoop, std::memory_order_relaxed);
  else
    resource_ptr->flags.fetch_and(~kLoop, std::memory_order_relaxed);
}

void AudioMixer::SetSimulateStereo(std::shared_ptr<void> resource,
                                   bool simulate) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  if (simulate)
    resource_ptr->flags.fetch_or(kSimulateStereo, std::memory_order_relaxed);
  else
    resource_ptr->flags.fetch_and(~kSimulateStereo, std::memory_order_relaxed);
}

void AudioMixer::SetResampleStep(std::shared_ptr<void> resource, size_t step) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  resource_ptr->step.store(step + 100, std::memory_order_relaxed);
}

void AudioMixer::SetMaxAmplitude(std::shared_ptr<void> resource,
                                 float max_amplitude) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  resource_ptr->max_amplitude.store(max_amplitude, std::memory_order_relaxed);
}

void AudioMixer::SetAmplitudeInc(std::shared_ptr<void> resource,
                                 float amplitude_inc) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  resource_ptr->amplitude_inc.store(amplitude_inc, std::memory_order_relaxed);
}

void AudioMixer::SetEndCallback(std::shared_ptr<void> resource,
                                base::Closure cb) {
  auto resource_ptr = std::static_pointer_cast<Resource>(resource);
  resource_ptr->end_cb = std::move(cb);
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
      play_list_[1].splice(play_list_[1].end(), play_list_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = play_list_[1].begin(); it != play_list_[1].end();) {
    auto audio_bus = (*it)->audio_bus.get();
    unsigned flags = (*it)->flags.load(std::memory_order_relaxed);
    bool marked_for_removal = false;

    if (flags & kStopped) {
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
      size_t channel_offset =
          (flags & kSimulateStereo) ? audio_bus->sample_rate() / 10 : 0;

      DCHECK(num_samples > 0);

      for (size_t i = 0; i < num_frames * kChannelCount;) {
        if (src_index < num_samples) {
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
            marked_for_removal = !(flags & kLoop);
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
                std::bind(&AudioMixer::DoStream, this, *it, flags & kLoop),
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
      end_list_.push_back(*it);
      it = play_list_[1].erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = end_list_.begin(); it != end_list_.end();) {
    if (!(*it)->streaming_in_progress.load(std::memory_order_relaxed)) {
      main_thread_task_runner_->PostTask(
          HERE, std::bind(&AudioMixer::EndCallback, this, *it));
      it = end_list_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioMixer::DoStream(std::shared_ptr<Resource> resource, bool loop) {
  resource->audio_bus->Stream(loop);
  resource->streaming_in_progress.store(false, std::memory_order_release);
}

void AudioMixer::EndCallback(std::shared_ptr<Resource> resource) {
  resource->active = false;

  if (resource->end_cb)
    resource->end_cb();
  if (resource->restart_cb) {
    resource->restart_cb();
    resource->restart_cb = nullptr;
  }
}

}  // namespace eng
