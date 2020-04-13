#ifndef GAME_FACTORY_H
#define GAME_FACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <utility>

#define DECLARE_GAME_BEGIN std::vector<std::pair<std::string, engine::GameFactoryBase*>> engine::GameFactoryBase::game_classes = {
#define DECLARE_GAME(CLASS) { #CLASS, new engine::GameFactory<CLASS>() },
#define DECLARE_GAME_END };

namespace engine {

class Game;

class GameFactoryBase {
 public:
  virtual ~GameFactoryBase() = default;

  static std::unique_ptr<Game> CreateGame(const std::string &name) {
    if (name.empty())
      return game_classes.size() > 0 ? game_classes.begin()->second->CreateGame() : nullptr;
    for (auto &element : game_classes) {
      if (element.first == name)
        return element.second->CreateGame();
    }
    return nullptr;
  }

 private:
  virtual std::unique_ptr<Game> CreateGame() { return nullptr; }

  static std::vector<std::pair<std::string, GameFactoryBase*>> game_classes;
};

template<typename Type>
class GameFactory : public GameFactoryBase {
 public:
  ~GameFactory() override = default;

 private:
  using GameType = Type;

  std::unique_ptr<Game> CreateGame() override {
    return std::make_unique<GameType>();
  }
};

} // namespace engine

#endif // GAME_FACTORY_H
