#include "engine/audio/audio_device_wasapi.h"

#include "base/log.h"

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

using namespace base;

namespace eng {

AudioDeviceWASAPI::AudioDeviceWASAPI(AudioDevice::Delegate* delegate)
    : delegate_(delegate) {}

AudioDeviceWASAPI::~AudioDeviceWASAPI() {
  LOG(0) << "Shutting down audio.";

  TerminateAudioThread();

  if (shutdown_event_)
    CloseHandle(shutdown_event_);
  if (ready_event_)
    CloseHandle(ready_event_);
  if (device_)
    device_->Release();
  if (audio_client_)
    audio_client_->Release();
  if (render_client_)
    render_client_->Release();
  if (device_enumerator_)
    device_enumerator_->Release();
}

bool AudioDeviceWASAPI::Initialize() {
  LOG(0) << "Initializing audio.";

  HRESULT hr;
  do {
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          IID_IMMDeviceEnumerator, (void**)&device_enumerator_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to instantiate device enumerator: " << hr;
      break;
    }

    hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, eConsole,
                                                     &device_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to get default audio endpoint: " << hr;
      break;
    }

    hr = device_->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                           (void**)&audio_client_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to activate audio client: " << hr;
      break;
    }

    // Use float format.
    WAVEFORMATEX* closest_match = nullptr;
    WAVEFORMATEXTENSIBLE wfxex = {0};
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wfxex.Format.nChannels = 2;
    wfxex.Format.nSamplesPerSec = 48000;
    wfxex.Format.wBitsPerSample = 32;
    wfxex.Samples.wValidBitsPerSample = 32;
    wfxex.Format.nBlockAlign =
        wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec =
        wfxex.Format.nBlockAlign * wfxex.Format.nSamplesPerSec;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    hr = audio_client_->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                          &wfxex.Format, &closest_match);
    if (FAILED(hr)) {
      LOG(0) << "Unsupported sample format.";
      break;
    }

    WAVEFORMATEX* format = closest_match ? closest_match : &wfxex.Format;
    if ((format->wFormatTag != WAVE_FORMAT_IEEE_FLOAT &&
         (format->wFormatTag != WAVE_FORMAT_EXTENSIBLE ||
          reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat !=
              KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) ||
        format->nChannels != 2) {
      LOG(0) << "Unsupported sample format.";
      break;
    }

    sample_rate_ = format->nSamplesPerSec;

    HRESULT hr = audio_client_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, 0, 0,
        format, NULL);
    if (closest_match)
      CoTaskMemFree(closest_match);
    if (FAILED(hr)) {
      LOG(0) << "Unable to initialize audio client: " << hr;
      break;
    }

    // Get the actual size of the allocated buffer.
    hr = audio_client_->GetBufferSize(&buffer_size_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to get audio client buffer size: " << hr;
      break;
    }

    hr = audio_client_->GetService(IID_IAudioRenderClient,
                                   (void**)&render_client_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to get audio render client: " << hr;
      break;
    }

    shutdown_event_ =
        CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (shutdown_event_ == NULL) {
      LOG(0) << "Unable to create shutdown event: " << hr;
      break;
    }
    ready_event_ =
        CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (ready_event_ == NULL) {
      LOG(0) << "Unable to create samples ready event: " << hr;
      break;
    }

    hr = audio_client_->SetEventHandle(ready_event_);
    if (FAILED(hr)) {
      LOG(0) << "Unable to set ready event: " << hr;
      break;
    }

    StartAudioThread();

    hr = audio_client_->Start();
    if (FAILED(hr)) {
      break;
    }

    return true;
  } while (false);

  return false;
}

void AudioDeviceWASAPI::Suspend() {
  audio_client_->Stop();
}

void AudioDeviceWASAPI::Resume() {
  audio_client_->Start();
}

size_t AudioDeviceWASAPI::GetHardwareSampleRate() {
  return sample_rate_;
}

void AudioDeviceWASAPI::StartAudioThread() {
  DCHECK(!audio_thread_.joinable());

  LOG(0) << "Starting audio thread.";
  audio_thread_ = std::thread(&AudioDeviceWASAPI::AudioThreadMain, this);
}

void AudioDeviceWASAPI::TerminateAudioThread() {
  LOG(0) << "Terminating audio thread";
  SetEvent(shutdown_event_);
  audio_thread_.join();
}

void AudioDeviceWASAPI::AudioThreadMain() {
  DCHECK(delegate_);

  HANDLE wait_array[] = {shutdown_event_, ready_event_};

  for (;;) {
    switch (WaitForMultipleObjects(2, wait_array, FALSE, INFINITE)) {
      case WAIT_OBJECT_0 + 0:  // shutdown_event_
        return;

      case WAIT_OBJECT_0 + 1: {  // ready_event_
        UINT32 padding;
        HRESULT hr = audio_client_->GetCurrentPadding(&padding);
        if (SUCCEEDED(hr)) {
          BYTE* pData;
          UINT32 frames_available = buffer_size_ - padding;
          hr = render_client_->GetBuffer(frames_available, &pData);
          if (SUCCEEDED(hr)) {
            delegate_->RenderAudio(reinterpret_cast<float*>(pData),
                                   frames_available);
            hr = render_client_->ReleaseBuffer(frames_available, 0);
            if (!SUCCEEDED(hr))
              DLOG(0) << "Unable to release buffer: " << hr;
          } else {
            DLOG(0) << "Unable to get buffer: " << hr;
          }
        }
      } break;
    }
  }
}

}  // namespace eng
