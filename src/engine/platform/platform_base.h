#ifndef PLATFORM_BASE_H
#define PLATFORM_BASE_H

#include <exception>
#include <memory>
#include <string>

#include "../../base/timer.h"
#include "../../base/worker.h"
#include "../audio/audio_forward.h"

namespace eng {

class Renderer;
class Engine;

class PlatformBase {
 public:
  void Initialize();

  void Shutdown();

  void RunMainLoop();

  int GetDeviceDpi() const { return device_dpi_; }

  const std::string& GetRootPath() const { return root_path_; }

  const std::string& GetDataPath() const { return data_path_; }

  const std::string& GetSharedDataPath() const { return shared_data_path_; }

  bool mobile_device() const { return mobile_device_; }

  static class InternalError : public std::exception {
  } internal_error;

 protected:
  base::Timer timer_;

  bool mobile_device_ = false;
  int device_dpi_ = 100;
  std::string root_path_;
  std::string data_path_;
  std::string shared_data_path_;

  bool has_focus_ = false;
  bool should_exit_ = false;

  std::unique_ptr<Audio> audio_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Engine> engine_;

  base::Worker worker_;

  PlatformBase();
  ~PlatformBase();

  PlatformBase(const PlatformBase&) = delete;
  PlatformBase& operator=(const PlatformBase&) = delete;
};

}  // namespace eng

#endif  // PLATFORM_BASE_H
