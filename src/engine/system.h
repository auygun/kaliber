#ifndef ENGINE_SYSTEM_H
#define ENGINE_SYSTEM_H

namespace eng {

class World;

// Base class for all systems
class System {
 public:
  virtual ~System() = default;

  virtual void Init(World& world) = 0;
  virtual void FixedUpdate(World& world) = 0;
  virtual void Update(World& world, float delta_time) = 0;
};

}  // namespace eng

#endif  // ENGINE_SYSTEM_H
