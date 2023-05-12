#ifndef ENGINE_GAME_FACTORY_H
#define ENGINE_GAME_FACTORY_H

#include <memory>
#include <string>
#include <vector>

#define DECLARE_GAME_BEGIN                                   \
  std::vector<std::pair<std::string, eng::GameFactoryBase*>> \
      eng::GameFactoryBase::game_classes = {
#define DECLARE_GAME(CLASS) {#CLASS, new eng::GameFactory<CLASS>()},
#define DECLARE_GAME_END };

namespace eng {

class Game;

class GameFactoryBase {
 public:
  virtual ~GameFactoryBase() = default;

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
