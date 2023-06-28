#include "engine/audio/audio_sink_oboe.h"

#include "base/log.h"
#include "third_party/oboe/include/oboe/Oboe.h"

using namespace base;

namespace eng {

AudioSinkOboe::AudioSinkOboe(AudioSink::Delegate* delegate)
    : callback_(std::make_unique<StreamCallback>(this)), delegate_(delegate) {}

AudioSinkOboe::~AudioSinkOboe() {
  LOG(0) << "Shutting down audio.";
  stream_->stop();
}

bool AudioSinkOboe::Initialize() {
  LOG(0) << "Initializing audio.";
  return RestartStream();
}

void AudioSinkOboe::Suspend() {
  stream_->pause();
}

void AudioSinkOboe::Resume() {
  stream_->start();
}

size_t AudioSinkOboe::GetHardwareSampleRate() {
  return stream_->getSampleRate();
}

AudioSinkOboe::StreamCallback::StreamCallback(AudioSinkOboe* audio_sink)
    : audio_sink_(audio_sink) {}

AudioSinkOboe::StreamCallback::~StreamCallback() = default;

oboe::DataCallbackResult AudioSinkOboe::StreamCallback::onAudioReady(
    oboe::AudioStream* oboe_stream,
    void* audio_data,
    int32_t num_frames) {
  float* output_buffer = static_cast<float*>(audio_data);
  audio_sink_->delegate_->RenderAudio(output_buffer, num_frames);
  return oboe::DataCallbackResult::Continue;
}

void AudioSinkOboe::StreamCallback::onErrorAfterClose(
    oboe::AudioStream* oboe_stream,
    oboe::Result error) {
  LOG(0) << "Error after close. Error: " << oboe::convertToText(error);

  audio_sink_->RestartStream();
}

bool AudioSinkOboe::RestartStream() {
  oboe::AudioStreamBuilder builder;
  oboe::Result result =
      builder.setSharingMode(oboe::SharingMode::Exclusive)
          ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
          ->setFormat(oboe::AudioFormat::Float)
          ->setChannelCount(delegate_->GetChannelCount())
          ->setDirection(oboe::Direction::Output)
          ->setUsage(oboe::Usage::Game)
          ->setCallback(callback_.get())
          ->openManagedStream(stream_);

  LOG(0) << "Oboe Audio Stream:";
  LOG(0) << "  performance mode: " << (int)stream_->getPerformanceMode();
  LOG(0) << "  format:           " << (int)stream_->getFormat();
  LOG(0) << "  channel count:    " << stream_->getChannelCount();
  LOG(0) << "  sample rate:      " << stream_->getSampleRate();

  if (result != oboe::Result::OK) {
    LOG(0) << "Failed to create the playback stream. Error: "
           << oboe::convertToText(result);
    return false;
  }

  stream_->start();
  return true;
}

}  // namespace eng
