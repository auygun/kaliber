#include "shader_source.h"

#include <cstring>

#include "../base/log.h"
#include "engine.h"
#include "platform/asset_file.h"

namespace eng {

bool ShaderSource::Load(const std::string& name) {
  Engine& engine = Engine::Get();

  name_ = name;

  std::string vertex_file_name = name;
  vertex_file_name += "_vertex";
  auto vertex_source = AssetFile::ReadWholeFile(vertex_file_name.c_str(),
                                                engine.GetRootPath().c_str(),
                                                &vertex_source_size_, true);
  if (!vertex_source) {
    LOG << "Failed to read file: " << vertex_file_name;
    return false;
  }

  vertex_source_ = std::move(vertex_source);

  std::string fragment_file_name = name;
  fragment_file_name += "_fragment";
  auto fragment_source = AssetFile::ReadWholeFile(fragment_file_name.c_str(),
                                                  engine.GetRootPath().c_str(),
                                                  &fragment_source_size_, true);
  if (!fragment_source) {
    LOG << "Failed to read file: " << fragment_file_name;
    return false;
  }

  LOG << "Loaded " << name;

  fragment_source_ = std::move(fragment_source);

  return true;
}

}  // namespace eng
