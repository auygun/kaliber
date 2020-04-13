#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

class GameObject {
 public:
  virtual ~GameObject() = default;

  virtual bool Initialize() = 0;

  virtual void Shutdown() = 0;

  virtual void Update(float delta_time) = 0;

  virtual void Draw(float frame_frac) = 0;
};

#endif // GAME_OBJECT_H
