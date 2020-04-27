#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

class GameObject {
 public:
  GameObject() = default;
  virtual ~GameObject() = default;

  virtual bool Initialize() = 0;

  virtual void Update(float delta_time) = 0;
};

#endif  // GAME_OBJECT_H
