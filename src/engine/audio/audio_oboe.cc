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
      ->setSampleRate(48000)
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

void AudioOboe::Play(std::shared_ptr<const Sound> sound,
                     std::shared_ptr<void> impl_data,
                     bool loop,
                     size_t step) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (sample->active)
    return;

  // The given sample is not accessed by the audio thread right now. It's safe
  // to write.
  *sample = {sound, 0, step, 0, (unsigned)(loop ? kLoop : 0), true};

  std::unique_lock<std::mutex> scoped_lock(mutex_);
  samples_[0].push_back(sample);
}

void AudioOboe::Stop(std::shared_ptr<void> impl_data) {
  auto sample = std::static_pointer_cast<Sample>(impl_data);
  if (!sample->active)
    return;

  // Audio thread does read-only access to "flags". It's safe to write here.
  sample->flags |= kStop;
}

void AudioOboe::RenderAudio(float *output_buffer, int32_t num_frames) {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    samples_[1].splice(samples_[1].end(), samples_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = samples_[1].begin(); it != samples_[1].end();) {
    Sample* sample = it->get();

    const float *src = sample->sound->GetBuffer();
    size_t num_samples = sample->sound->num_samples();
    int num_channels = sample->sound->num_channels();
    int src_channel_step = sample->sound->num_channels() - 1;
    bool remove = false;

    if (sample->flags & kStop) {
      remove = true;
    } else {
      for (size_t i = 0; i < num_frames * kChannelCount;) {
        // Mix channels.
        output_buffer[i++] += src[sample->src_index];
        sample->src_index += src_channel_step;
        output_buffer[i++] += src[sample->src_index];

        // Basic resampling.
        sample->accumulator += sample->step;
        sample->src_index += num_channels * sample->accumulator / 10;
        sample->accumulator %= 10;

        if (sample->flags & kLoop) {
          sample->src_index %= num_samples;
        } else if (sample->src_index >= num_samples) {
          remove = true;
          break;
        }
      }
    }

    if (remove) {
      // Main thread does read-only access to "active". It's safe to write here.
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
  audio_->Initialize();
}

}  // namespace eng
