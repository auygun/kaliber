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

AudioMixer::~AudioMixer() = default;

uint64_t AudioMixer::CreateResource() {
  uint64_t resource_id = ++last_resource_id_;
  resources_[resource_id] = std::make_shared<Resource>();
  return resource_id;
}

void AudioMixer::DestroyResource(uint64_t resource_id) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (it->second->active) {
    it->second->restart_cb = nullptr;
    it->second->flags.fetch_or(kStopped, std::memory_order_relaxed);
  }
  resources_.erase(it);
}

void AudioMixer::Play(uint64_t resource_id,
                      std::shared_ptr<AudioBus> audio_bus,
                      float amplitude,
                      bool reset_pos) {
  if (!audio_enabled_)
    return;

  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (it->second->active) {
    if (reset_pos)
      it->second->flags.fetch_or(kStopped, std::memory_order_relaxed);

    if (it->second->flags.load(std::memory_order_relaxed) & kStopped)
      it->second->restart_cb = [&, resource_id, audio_bus, amplitude,
                                reset_pos]() -> void {
        Play(resource_id, audio_bus, amplitude, reset_pos);
      };

    return;
  }

  if (reset_pos) {
    it->second->src_index = 0;
    it->second->accumulator = 0;
    audio_bus->ResetStream();
  } else if (it->second->src_index >= audio_bus->samples_per_channel()) {
    return;
  }

  it->second->active = true;
  it->second->flags.fetch_and(~kStopped, std::memory_order_relaxed);
  it->second->audio_bus = audio_bus;
  if (amplitude >= 0)
    it->second->amplitude = amplitude;

  std::lock_guard<std::mutex> scoped_lock(lock_);
  play_list_[0].push_back(it->second);
}

void AudioMixer::Stop(uint64_t resource_id) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (it->second->active) {
    it->second->restart_cb = nullptr;
    it->second->flags.fetch_or(kStopped, std::memory_order_relaxed);
  }
}

void AudioMixer::SetLoop(uint64_t resource_id, bool loop) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (loop)
    it->second->flags.fetch_or(kLoop, std::memory_order_relaxed);
  else
    it->second->flags.fetch_and(~kLoop, std::memory_order_relaxed);
}

void AudioMixer::SetSimulateStereo(uint64_t resource_id, bool simulate) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (simulate)
    it->second->flags.fetch_or(kSimulateStereo, std::memory_order_relaxed);
  else
    it->second->flags.fetch_and(~kSimulateStereo, std::memory_order_relaxed);
}

void AudioMixer::SetResampleStep(uint64_t resource_id, size_t step) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->step.store(step + 100, std::memory_order_relaxed);
}

void AudioMixer::SetMaxAmplitude(uint64_t resource_id, float max_amplitude) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->max_amplitude.store(max_amplitude, std::memory_order_relaxed);
}

void AudioMixer::SetAmplitudeInc(uint64_t resource_id, float amplitude_inc) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->amplitude_inc.store(amplitude_inc, std::memory_order_relaxed);
}

void AudioMixer::SetEndCallback(uint64_t resource_id, base::Closure cb) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->end_cb = std::move(cb);
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
    auto audio_bus = it->get()->audio_bus.get();
    unsigned flags = it->get()->flags.load(std::memory_order_relaxed);
    bool marked_for_removal = false;

    if (flags & kStopped) {
      marked_for_removal = true;
    } else {
      const float* src[2] = {audio_bus->GetChannelData(0),
                             audio_bus->GetChannelData(1)};
      if (!src[1])
        src[1] = src[0];  // mono.

      size_t num_samples = audio_bus->samples_per_channel();
      size_t src_index = it->get()->src_index;
      size_t step = it->get()->step.load(std::memory_order_relaxed);
      size_t accumulator = it->get()->accumulator;
      float amplitude = it->get()->amplitude;
      float amplitude_inc =
          it->get()->amplitude_inc.load(std::memory_order_relaxed);
      float max_amplitude =
          it->get()->max_amplitude.load(std::memory_order_relaxed);
      size_t channel_offset =
          (flags & kSimulateStereo) ? audio_bus->sample_rate() / 10 : 0;

      DCHECK(num_samples > 0);

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
          marked_for_removal = true;
          break;
        } else if (amplitude > max_amplitude) {
          amplitude = max_amplitude;
        }

        // Advance source index. Apply basic resampling for variations.
        accumulator += step;
        src_index += accumulator / 100;
        accumulator %= 100;

        // Remove, loop or stream if the source data is consumed
        if (src_index >= num_samples) {
          src_index %= num_samples;

          if (audio_bus->EndOfStream()) {
            marked_for_removal = !(flags & kLoop);
            break;
          }

          if (!it->get()->streaming_in_progress.load(
                  std::memory_order_acquire)) {
            it->get()->streaming_in_progress.store(true,
                                                   std::memory_order_relaxed);

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
            DLOG << "Mixer buffer underrun!";
          }
        }
      }

      it->get()->src_index = src_index;
      it->get()->accumulator = accumulator;
      it->get()->amplitude = amplitude;
    }

    if (marked_for_removal) {
      end_list_.push_back(*it);
      it = play_list_[1].erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = end_list_.begin(); it != end_list_.end();) {
    if (!it->get()->streaming_in_progress.load(std::memory_order_relaxed)) {
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
