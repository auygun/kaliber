#include "audio_oboe.h"

#include <cstring>

#include "../../base/log.h"
#include "../../base/random.h"
#include "../../third_party/oboe/include/oboe/Oboe.h"

namespace eng {

base::Random rrr;

AudioOboe::AudioOboe() : callback_(std::make_unique<StreamCallback>(this)) {}

AudioOboe::~AudioOboe() = default;

bool AudioOboe::Initialize() {
  LOG << "Initializing audio system.";

  oboe::AudioStreamBuilder builder;
  oboe::Result result =  builder.setSharingMode(oboe::SharingMode::Exclusive)
      ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
      ->setFormat(oboe::AudioFormat::Float)
      ->setChannelCount(1)
      ->setCallback(callback_.get())
      ->openManagedStream(stream_);

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

AudioOboe::StreamCallback::StreamCallback(AudioOboe* audio) : audio_(audio) {}

AudioOboe::StreamCallback::~StreamCallback() = default;

oboe::DataCallbackResult AudioOboe::StreamCallback::onAudioReady(
    oboe::AudioStream *oboe_stream,
    void *audio_data,
    int32_t num_frames) {
  float *output_buffer = static_cast<float*>(audio_data);
  memset(output_buffer, 0, sizeof(float) * num_frames);
  for (int i = 0; i < num_frames; ++i)
    output_buffer[i] += rrr.GetFloat() * (rrr.Roll(2) == 2 ? 1.0f : -1.0f);

return oboe::DataCallbackResult::Continue;
}

void AudioOboe::StreamCallback::onErrorAfterClose(
    oboe::AudioStream *oboe_stream, oboe::Result error) {
  LOG << "Error after close. Error: %s" << oboe::convertToText(error);
  audio_->Initialize();
}

}  // namespace eng
