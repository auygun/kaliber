#include "audio_oboe.h"

#include <cstring>

#include "../../base/interpolation.h"
#include "../../base/log.h"
#include "../../base/random.h"
#include "../../third_party/oboe/include/oboe/Oboe.h"
#include "../sound.h"
#include "audio_resource.h"

using namespace base;

namespace {

constexpr int kChannelCount = 2;

}  // namespace

namespace eng {

AudioOboe::AudioOboe() : callback_(std::make_unique<StreamCallback>(this)) {}

AudioOboe::~AudioOboe() = default;

bool AudioOboe::Initialize() {
  LOG << "Initializing audio system.";

  oboe::AudioStreamBuilder builder;
  oboe::Result result =  builder.setSharingMode(oboe::SharingMode::Exclusive)
      ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
      ->setFormat(oboe::AudioFormat::Float)
      ->setChannelCount(kChannelCount)
      ->setDirection(oboe::Direction::Output)
      ->setUsage(oboe::Usage::Game)
      ->setCallback(callback_.get())
      ->openManagedStream(stream_);

  LOG << "Oboe Audio Stream:";
  LOG << "  performance mode: " << (int)stream_->getPerformanceMode();
  LOG << "  format:           " << (int)stream_->getFormat();
  LOG << "  channel count:    " << stream_->getChannelCount();
  LOG << "  sample rate:      " << stream_->getSampleRate();

  if (result != oboe::Result::OK) {
    LOG << "Failed to create the playback stream. Error: %s"
        << oboe::convertToText(result);
    return false;
  }

  stream_->start();

  return true;
}

void AudioOboe::Shutdown() {
  LOG << "Shutting down audio system.";
}

std::shared_ptr<AudioResource> AudioOboe::CreateResource() {
  auto impl_data = std::make_shared<Sample>();
  return std::make_shared<AudioResource>(impl_data, this);
}

void AudioOboe::Play(std::shared_ptr<void> impl_data,
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

void AudioOboe::Stop(std::shared_ptr<void> impl_data) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (!sample->active)
    return;

  sample->flags |= kStopped;
}

void AudioOboe::SetLoop(std::shared_ptr<void> impl_data, bool loop) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (loop)
    sample->flags |= kLoop;
  else
    sample->flags &= ~kLoop;
}

void AudioOboe::SetSimulateStereo(std::shared_ptr<void> impl_data,
                                  bool simulate) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (simulate)
    sample->flags |= kSimulateStereo;
  else
    sample->flags &= ~kSimulateStereo;
}

void AudioOboe::SetResampleStep(std::shared_ptr<void> impl_data, size_t step) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->step = step + 10;
}

void AudioOboe::SetMaxAmplitude(std::shared_ptr<void> impl_data, float max_amplitude) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->max_amplitude = max_amplitude;
}

void AudioOboe::SetAmplitudeInc(std::shared_ptr<void> impl_data,
                                float amplitude_inc) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  sample->amplitude_inc = amplitude_inc;
}

size_t AudioOboe::GetSampleRate() {
  return stream_->getSampleRate();
}

void AudioOboe::RenderAudio(float *output_buffer, int32_t num_frames) {
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
      const float *src[2] = {sample->sound->GetBuffer(0),
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
      sample->active = false;
      it = samples_[1].erase(it);
    } else {
      ++it;
    }
  }
}

AudioOboe::StreamCallback::StreamCallback(AudioOboe* audio) : audio_(audio) {}

AudioOboe::StreamCallback::~StreamCallback() = default;

oboe::DataCallbackResult AudioOboe::StreamCallback::onAudioReady(
    oboe::AudioStream *oboe_stream,
    void *audio_data,
    int32_t num_frames) {
  float *output_buffer = static_cast<float*>(audio_data);
  audio_->RenderAudio(output_buffer, num_frames);
  return oboe::DataCallbackResult::Continue;
}

void AudioOboe::StreamCallback::onErrorAfterClose(
    oboe::AudioStream *oboe_stream, oboe::Result error) {
  LOG << "Error after close. Error: %s" << oboe::convertToText(error);
  // TODO: Do this in main thread.
  audio_->Initialize();
}

}  // namespace eng
