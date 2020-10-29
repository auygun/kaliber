#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <string>

#include "../base/log.h"
#include "../third_party/jsoncpp/json.h"

namespace eng {

namespace internal {

template <typename T>
T Get(const Json::Value& val, T default_val) {
  NOTREACHED;
}

// Explicit specialization for basic types.
template <>
unsigned Get(const Json::Value& val, unsigned default_val);
template <>
int Get(const Json::Value& val, int default_val);
template <>
bool Get(const Json::Value& val, bool default_val);

}  // namespace internal

class PersistentData {
 public:
  enum StorageType { kPrivate, kShared, kAsset };

  PersistentData() = default;
  ~PersistentData() = default;

  bool Load(const std::string& file_name, StorageType type = kPrivate);

  bool Save(bool force = false);

  bool SaveAs(const std::string& file_name, StorageType type = kPrivate);

  template <typename T>
  T Get(const char* key, T default_val) const {
    Json::Value val = root_.get(key, Json::Value());
    return internal::Get<T>(val, default_val);
  }

  Json::Value& operator[](const char* key);
  Json::Value& operator[](const std::string& key);

  const Json::Value& operator[](const char* key) const;
  const Json::Value& operator[](const std::string& key) const;

 private:
  StorageType type_ = kPrivate;
  std::string file_name_;
  Json::Value root_;
  bool dirty_ = false;
};

template <typename T>
Json::Value& operator<<(Json::Value& val, const T& arg) {
  val = arg;
  return val;
}

template <typename T>
const Json::Value& operator>>(const Json::Value& val, T& arg) {
  NOTREACHED;
}

// Explicit specialization for basic types.
template <>
const Json::Value& operator>>(const Json::Value& val, unsigned& arg);
template <>
const Json::Value& operator>>(const Json::Value& val, int& arg);
template <>
const Json::Value& operator>>(const Json::Value& val, bool& arg);

}  // namespace eng

#endif  // SAVE_GAME_H
