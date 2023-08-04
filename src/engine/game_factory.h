#ifndef ENGINE_GAME_FACTORY_H
#define ENGINE_GAME_FACTORY_H

#include <memory>
#include <string>
#include <vector>

#define GAME_FACTORIES                                       \
  std::vector<std::pair<std::string, eng::GameFactoryBase*>> \
      eng::GameFactoryBase::game_classes
#define GAME_CLASS(CLASS) \
  { #CLASS, new eng::GameFactory<CLASS>() }

namespace eng {

class Game;

class GameFactoryBase {
 public:
  virtual ~GameFactoryBase() = default;

  // Create an instance for the class of the given name. The default factory is
  // used if the name is empty (which is the first one in the list).
  static std::unique_ptr<Game> CreateGame(const std::string& name) {
    if (name.empty())
      return game_classes.size() > 0
                 ? game_classes.begin()->second->CreateGame()
                 : nullptr;
    for (auto& element : game_classes) {
      if (element.first == name)
        return element.second->CreateGame();
    }
    return nullptr;
  }

 private:
  virtual std::unique_ptr<Game> CreateGame() { return nullptr; }

  static std::vector<std::pair<std::string, GameFactoryBase*>> game_classes;
};

template <typename Type>
class GameFactory final : public GameFactoryBase {
 public:
  ~GameFactory() final = default;

 private:
  using GameType = Type;

  std::unique_ptr<Game> CreateGame() final {
    return std::make_unique<GameType>();
  }
};

}  // namespace eng

#endif  // ENGINE_GAME_FACTORY_H
