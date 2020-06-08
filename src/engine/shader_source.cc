#include "shader_source.h"

#include <cstring>

#include "../base/asset_file.h"
#include "../base/log.h"
#include "engine.h"

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

  vertex_source_ = std::string(vertex_source.get(), size + 1);

  std::string fragment_file_name = name;
  fragment_file_name += "_fragment";
  auto fragment_source = base::AssetFile::ReadWholeFile(
      fragment_file_name.c_str(), engine.GetRootPath().c_str(), &size, true);
  if (!fragment_source) {
    LOG << "Failed to read file: " << fragment_file_name;
    return false;
  }

  fragment_source_ = std::string(fragment_source.get(), size + 1);

  return true;
}

}  // namespace eng
