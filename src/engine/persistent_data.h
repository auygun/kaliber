#ifndef ENGINE_SAVE_GAME_H
#define ENGINE_SAVE_GAME_H

#include <string>

#include "base/log.h"
#include "third_party/jsoncpp/json.h"

namespace eng {

class PersistentData {
 public:
  enum StorageType { kPrivate, kShared, kAsset };

  PersistentData() = default;
  ~PersistentData() = default;

  bool Load(const std::string& file_name, StorageType type = kPrivate);

  bool Save(bool force = false);

  bool SaveAs(const std::string& file_name, StorageType type = kPrivate);

  Json::Value& root() {
    dirty_ = true;
    return root_;
  }

  const Json::Value& root() const { return root_; }

 private:
  StorageType type_ = kPrivate;
  std::string file_name_;
  Json::Value root_;
  bool dirty_ = false;
};

}  // namespace eng

#endif  // ENGINE_SAVE_GAME_H
