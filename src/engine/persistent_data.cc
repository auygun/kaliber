#include "persistent_data.h"

#include <memory>

#include "../base/file.h"
#include "engine.h"

using namespace base;

namespace eng {

bool PersistentData::Load(const std::string& file_name, bool shared) {
  file_name_ = file_name;
  shared_ = shared;

  std::string file_path =
      shared ? Engine::Get().GetSharedDataPath() : Engine::Get().GetDataPath();
  file_path += file_name;
  ScopedFILE file;
  file.reset(fopen(file_path.c_str(), "r"));
  if (!file) {
    LOG << "Failed to open file " << file_path;
    return false;
  }

  size_t size = 0;
  if (file) {
    if (!fseek(file.get(), 0, SEEK_END)) {
      size = ftell(file.get());
      rewind(file.get());
    }
  }

  auto buffer = std::make_unique<char[]>(size + 1);
  size_t bytes_read = fread(buffer.get(), 1, size, file.get());
  if (!bytes_read) {
    LOG << "Failed to read a buffer of size: " << size << " from file "
        << file_path;
    return false;
  }
  buffer[size] = 0;

  std::string err;
  Json::CharReaderBuilder builder;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(buffer.get(), buffer.get() + size, &root_, &err)) {
    LOG << "Failed to parse save file. Json parser error: " << err;
    return false;
  }

  dirty_ = false;

  return true;
}

bool PersistentData::Save(bool force) {
  if (!force && !dirty_)
    return true;

  DCHECK(!file_name_.empty());

  return SaveAs(file_name_, shared_);
}

bool PersistentData::SaveAs(const std::string& file_name, bool shared) {
  file_name_ = file_name;

  Json::StreamWriterBuilder builder;
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  std::ostringstream stream;
  writer->write(root_, &stream);

  std::string file_path =
      shared ? Engine::Get().GetSharedDataPath() : Engine::Get().GetDataPath();
  file_path += file_name;
  ScopedFILE file;
  file.reset(fopen(file_path.c_str(), "w"));
  if (!file) {
    LOG << "Failed to create file " << file_path;
    return false;
  }

  std::string data = stream.str();
  if (fwrite(data.c_str(), data.size(), 1, file.get()) != 1) {
    LOG << "Failed to write to file " << file_path;
    return false;
  }

  dirty_ = false;

  return true;
}

Json::Value& PersistentData::operator[](const char* key) {
  dirty_ = true;
  return root_[key];
}

Json::Value& PersistentData::operator[](const std::string& key) {
  dirty_ = true;
  return root_[key];
}

const Json::Value& PersistentData::operator[](const char* key) const {
  return root_[key];
}

const Json::Value& PersistentData::operator[](const std::string& key) const {
  return root_[key];
}

template <>
const Json::Value& operator>>(const Json::Value& val, unsigned& arg) {
  if (!val.isNull() && val.isConvertibleTo(Json::uintValue))
    arg = val.asUInt();
  return val;
}

template <>
const Json::Value& operator>>(const Json::Value& val, int& arg) {
  if (!val.isNull() && val.isConvertibleTo(Json::intValue))
    arg = val.asInt();
  return val;
}

template <>
const Json::Value& operator>>(const Json::Value& val, bool& arg) {
  if (!val.isNull() && val.isConvertibleTo(Json::booleanValue))
    arg = val.asBool();
  return val;
}

namespace internal {

template <>
unsigned Get(const Json::Value& val, unsigned default_val) {
  return (!val.isNull() && val.isConvertibleTo(Json::uintValue)) ? val.asUInt()
                                                                 : default_val;
}

template <>
int Get(const Json::Value& val, int default_val) {
  return (!val.isNull() && val.isConvertibleTo(Json::intValue)) ? val.asInt()
                                                                : default_val;
}

template <>
bool Get(const Json::Value& val, bool default_val) {
  return (!val.isNull() && val.isConvertibleTo(Json::intValue)) ? val.asBool()
                                                                : default_val;
}

}  // namespace internal

}  // namespace eng
