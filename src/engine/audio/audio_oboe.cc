#include "audio_oboe.h"

#include "../../base/log.h"
#include "../../third_party/oboe/include/oboe/Oboe.h"

using namespace base;

namespace eng {

AudioOboe::AudioOboe() : callback_(std::make_unique<StreamCallback>(this)) {}

AudioOboe::~AudioOboe() = default;

bool AudioOboe::Initialize() {
  LOG << "Initializing audio system.";

  return RestartStream();
}

void AudioOboe::Shutdown() {
  LOG << "Shutting down audio system.";

  stream_->stop();
}

void AudioOboe::Suspend() {
  stream_->stop();
}

void AudioOboe::Resume() {
  RestartStream();
}

int AudioOboe::GetHardwareSampleRate() {
  return stream_->getSampleRate();
}

AudioOboe::StreamCallback::StreamCallback(AudioOboe* audio) : audio_(audio) {}

AudioOboe::StreamCallback::~StreamCallback() = default;

oboe::DataCallbackResult AudioOboe::StreamCallback::onAudioReady(
    oboe::AudioStream* oboe_stream,
    void* audio_data,
    int32_t num_frames) {
  float* output_buffer = static_cast<float*>(audio_data);
  audio_->RenderAudio(output_buffer, num_frames);
  return oboe::DataCallbackResult::Continue;
}

void AudioOboe::StreamCallback::onErrorAfterClose(
    oboe::AudioStream* oboe_stream,
    oboe::Result error) {
  LOG << "Error after close. Error: " << oboe::convertToText(error);

  audio_->RestartStream();
}

bool AudioOboe::RestartStream() {
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

  stream_->start();
  return true;
}

}  // namespace eng
