#include "engine/audio/audio.h"

#include <cstring>

#include "base/log.h"
#include "base/task_runner.h"
#include "base/thread_pool.h"
#include "engine/sound.h"

using namespace base;

namespace eng {

AudioBase::AudioBase()
    : main_thread_task_runner_(TaskRunner::GetThreadLocalTaskRunner()) {}

AudioBase::~AudioBase() = default;

uint64_t AudioBase::CreateResource() {
  uint64_t resource_id = ++last_resource_id_;
  resources_[resource_id] = std::make_shared<Resource>();
  return resource_id;
}

void AudioBase::DestroyResource(uint64_t resource_id) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->flags.fetch_or(kStopped, std::memory_order_relaxed);
  resources_.erase(it);
}

void AudioBase::Play(uint64_t resource_id,
                     std::shared_ptr<Sound> sound,
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

    if (it->second->flags.load(std::memory_order_relaxed) & kStopped) {
      Closure ocb = std::move(it->second->end_cb);
      SetEndCallback(
          resource_id,
          [&, resource_id, sound, amplitude, reset_pos, ocb]() -> void {
            Play(resource_id, sound, amplitude, reset_pos);
            SetEndCallback(resource_id, std::move(ocb));
          });
    }

    return;
  }

  if (reset_pos) {
    it->second->src_index = 0;
    it->second->accumulator = 0;
    sound->ResetStream();
  }

  it->second->active = true;
  it->second->flags.fetch_and(~kStopped, std::memory_order_relaxed);
  it->second->sound = sound;
  if (amplitude >= 0)
    it->second->amplitude = amplitude;

  std::lock_guard<std::mutex> scoped_lock(lock_);
  play_list_[0].push_back(it->second);
}

void AudioBase::Stop(uint64_t resource_id) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (it->second->active)
    it->second->flags.fetch_or(kStopped, std::memory_order_relaxed);
}

void AudioBase::SetLoop(uint64_t resource_id, bool loop) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (loop)
    it->second->flags.fetch_or(kLoop, std::memory_order_relaxed);
  else
    it->second->flags.fetch_and(~kLoop, std::memory_order_relaxed);
}

void AudioBase::SetSimulateStereo(uint64_t resource_id, bool simulate) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  if (simulate)
    it->second->flags.fetch_or(kSimulateStereo, std::memory_order_relaxed);
  else
    it->second->flags.fetch_and(~kSimulateStereo, std::memory_order_relaxed);
}

void AudioBase::SetResampleStep(uint64_t resource_id, size_t step) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->step.store(step + 100, std::memory_order_relaxed);
}

void AudioBase::SetMaxAmplitude(uint64_t resource_id, float max_amplitude) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->max_amplitude.store(max_amplitude, std::memory_order_relaxed);
}

void AudioBase::SetAmplitudeInc(uint64_t resource_id, float amplitude_inc) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->amplitude_inc.store(amplitude_inc, std::memory_order_relaxed);
}

void AudioBase::SetEndCallback(uint64_t resource_id, base::Closure cb) {
  auto it = resources_.find(resource_id);
  if (it == resources_.end())
    return;

  it->second->end_cb = std::move(cb);
}

void AudioBase::RenderAudio(float* output_buffer, size_t num_frames) {
  {
    std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
    if (scoped_lock)
      play_list_[1].splice(play_list_[1].end(), play_list_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = play_list_[1].begin(); it != play_list_[1].end();) {
    auto sound = it->get()->sound.get();
    unsigned flags = it->get()->flags.load(std::memory_order_relaxed);
    bool marked_for_removal = false;

    if (flags & kStopped) {
      marked_for_removal = true;
    } else {
      const float* src[2] = {sound->GetBuffer(0), sound->GetBuffer(1)};
      if (!src[1])
        src[1] = src[0];  // mono.

      size_t num_samples = sound->GetNumSamples();
      size_t src_index = it->get()->src_index;
      size_t step = it->get()->step.load(std::memory_order_relaxed);
      size_t accumulator = it->get()->accumulator;
      float amplitude = it->get()->amplitude;
      float amplitude_inc =
          it->get()->amplitude_inc.load(std::memory_order_relaxed);
      float max_amplitude =
          it->get()->max_amplitude.load(std::memory_order_relaxed);

      size_t channel_offset =
          (flags & kSimulateStereo) && !sound->is_streaming_sound()
              ? sound->sample_rate() / 10
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
        }

        // Remove, loop or stream if the source data is consumed
        if (src_index >= num_samples) {
          if (!sound->is_streaming_sound()) {
            src_index %= num_samples;

            if (!(flags & kLoop)) {
              marked_for_removal = true;
              break;
            }
          } else if (!it->get()->streaming_in_progress.load(
                         std::memory_order_acquire)) {
            if (num_samples)
              src_index %= num_samples;

            // Looping streaming sounds never return eof.
            if (sound->eof()) {
              marked_for_removal = true;
              break;
            }

            it->get()->streaming_in_progress.store(true,
                                                   std::memory_order_relaxed);

            // Swap buffers and start streaming in background.
            sound->SwapBuffers();
            src[0] = sound->GetBuffer(0);
            src[1] = sound->GetBuffer(1);
            if (!src[1])
              src[1] = src[0];  // mono.
            num_samples = sound->GetNumSamples();

            ThreadPool::Get().PostTask(
                HERE,
                std::bind(&AudioBase::DoStream, this, *it, flags & kLoop));
          } else if (num_samples) {
            DLOG << "Buffer underrun!";
            src_index %= num_samples;
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
    if ((!it->get()->sound->is_streaming_sound() ||
         !it->get()->streaming_in_progress.load(std::memory_order_relaxed))) {
      main_thread_task_runner_->PostTask(
          HERE, std::bind(&AudioBase::EndCallback, this, *it));
      it = end_list_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioBase::DoStream(std::shared_ptr<Resource> resource, bool loop) {
  resource->sound->Stream(loop);

  // Memory barrier to ensure all memory writes become visible to the audio
  // thread.
  resource->streaming_in_progress.store(false, std::memory_order_release);
}

void AudioBase::EndCallback(std::shared_ptr<Resource> resource) {
  resource->active = false;

  if (resource->end_cb)
    resource->end_cb();
}

}  // namespace eng
