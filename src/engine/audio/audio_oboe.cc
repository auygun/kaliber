#include "audio_oboe.h"

#include <cstring>

#include "../../base/log.h"
#include "../../third_party/oboe/include/oboe/Oboe.h"
#include "../sound.h"

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

  LOG << "Audio stream sample rate: " << stream_->getSampleRate();

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

void AudioOboe::Play(std::shared_ptr<const Sound> sound, bool loop) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  Sample &s = samples_[0].emplace_back();
  s = {sound, 0, (unsigned)(loop ? kLoop : 0)};
}

void AudioOboe::RenderAudio(float *output_buffer, int32_t num_frames) {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    samples_[1].splice(samples_[1].end(), samples_[0]);
  }

  memset(output_buffer, 0, sizeof(float) * num_frames * kChannelCount);

  for (auto it = samples_[1].begin(); it != samples_[1].end();) {
    const float *src = it->sound->GetBuffer();
    for (size_t i = 0; i < num_frames * kChannelCount; ++i) {
      output_buffer[i] += src[it->ind++];
      if (it->flags_ & kLoop) {
        it->ind %= it->sound->num_samples();
      } else if (it->ind >= it->sound->num_samples()) {
        it = samples_[1].erase(it);
        break;
      }
    }
    ++it;
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
