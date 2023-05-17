#include "engine/audio/audio_driver_oboe.h"

#include "base/log.h"
#include "engine/audio/audio_driver_delegate.h"
#include "third_party/oboe/include/oboe/Oboe.h"

using namespace base;

namespace eng {

AudioDriverOboe::AudioDriverOboe()
    : callback_(std::make_unique<StreamCallback>(this)) {}

AudioDriverOboe::~AudioDriverOboe() = default;

void AudioDriverOboe::SetDelegate(AudioDriverDelegate* delegate) {
  delegate_ = delegate;
  Resume();
}

bool AudioDriverOboe::Initialize() {
  LOG << "Initializing audio system.";

  return RestartStream(true);
}

void AudioDriverOboe::Shutdown() {
  LOG << "Shutting down audio system.";

  stream_->stop();
}

void AudioDriverOboe::Suspend() {
  stream_->stop();
}

void AudioDriverOboe::Resume() {
  RestartStream();
}

int AudioDriverOboe::GetHardwareSampleRate() {
  return stream_->getSampleRate();
}

AudioDriverOboe::StreamCallback::StreamCallback(AudioDriverOboe* driver)
    : driver_(driver) {}

AudioDriverOboe::StreamCallback::~StreamCallback() = default;

oboe::DataCallbackResult AudioDriverOboe::StreamCallback::onAudioReady(
    oboe::AudioStream* oboe_stream,
    void* audio_data,
    int32_t num_frames) {
  float* output_buffer = static_cast<float*>(audio_data);
  driver_->delegate_->RenderAudio(output_buffer, num_frames);
  return oboe::DataCallbackResult::Continue;
}

void AudioDriverOboe::StreamCallback::onErrorAfterClose(
    oboe::AudioStream* oboe_stream,
    oboe::Result error) {
  LOG << "Error after close. Error: " << oboe::convertToText(error);

  driver_->RestartStream();
}

bool AudioDriverOboe::RestartStream(bool suspended) {
  oboe::AudioStreamBuilder builder;
  oboe::Result result =
      builder.setSharingMode(oboe::SharingMode::Exclusive)
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
    LOG << "Failed to create the playback stream. Error: "
        << oboe::convertToText(result);
    return false;
  }

  if (!suspended)
    stream_->start();
  return true;
}

}  // namespace eng
