#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <string>

#include "../base/log.h"
#include "../third_party/jsoncpp/json.h"

namespace eng {

class PersistentData {
 public:
  PersistentData() = default;
  ~PersistentData() = default;

  bool Load(const std::string& file_name);

  bool Save(const std::string& file_name);

  Json::Value& operator[](const char* key);
  Json::Value& operator[](const std::string& key);

  const Json::Value& operator[](const char* key) const;
  const Json::Value& operator[](const std::string& key) const;

 private:
  Json::Value root_;
};

}  // namespace eng

#endif  // SAVE_GAME_H
