#include "shader_source.h"

#include <cstring>

#include "../base/asset_file.h"
#include "../base/log.h"
#include "engine.h"

namespace {

constexpr char kKeywordDelimiter[] = ";{}() \t";
constexpr char kKeywordHighp[] = "highp";

}  // namespace

namespace eng {

bool ShaderSource::Load(const std::string& name) {
  if (IsImmutable()) {
    LOG << "Error: ShaderSource is immutable. Failed to load.";
    return false;
  }

  Engine& engine = Engine::Get();

  size_t size = 0;

  std::string vertex_file_name = name;
  vertex_file_name += "_vertex";
  auto vertex_source = base::AssetFile::ReadWholeFile(
      vertex_file_name.c_str(), engine.GetRootPath().c_str(), &size, true);
  if (!vertex_source) {
    LOG << "Failed to read file: " << vertex_file_name;
    return false;
  }

  vertex_source_ = engine.IsMobile() ? std::move(vertex_source)
                                     : Preprocess(vertex_source.get(), size);

  std::string fragment_file_name = name;
  fragment_file_name += "_fragment";
  auto fragment_source = base::AssetFile::ReadWholeFile(
      fragment_file_name.c_str(), engine.GetRootPath().c_str(), &size, true);
  if (!fragment_source) {
    LOG << "Failed to read file: " << fragment_file_name;
    return false;
  }

  fragment_source_ = engine.IsMobile()
                         ? std::move(fragment_source)
                         : Preprocess(fragment_source.get(), size);

  return true;
}

std::unique_ptr<char[]> ShaderSource::Preprocess(char* source, size_t size) {
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
  char* dst = buffer.get();
  memcpy(dst, source, size + 1);

  char* token = strtok(source, kKeywordDelimiter);
  char* last_token = token;

  while (token) {
    dst += token - last_token;
    last_token = token;

    if (strcmp(token, kKeywordHighp) == 0) {
      size_t keyword_length = strlen(kKeywordHighp);
      memset(dst, ' ', keyword_length);
    }

    token = strtok(NULL, kKeywordDelimiter);
  }

  return buffer;
}

}  // namespace eng
