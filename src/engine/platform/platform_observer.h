#ifndef ENGINE_PLATFORM_PLATFORM_OBSERVER_H
#define ENGINE_PLATFORM_PLATFORM_OBSERVER_H

namespace eng {

class InputEvent;

class PlatformObserver {
 public:
  PlatformObserver() = default;
  virtual ~PlatformObserver() = default;

  virtual void OnWindowCreated() = 0;
  virtual void OnWindowDestroyed() = 0;
  virtual void OnWindowResized(int width, int height) = 0;
  virtual void LostFocus() = 0;
  virtual void GainedFocus(bool from_interstitial_ad) = 0;
  virtual void AddInputEvent(std::unique_ptr<InputEvent> event) = 0;
};

}  // namespace eng

#endif  // ENGINE_PLATFORM_PLATFORM_OBSERVER_H
