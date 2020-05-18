#include "shader_source.h"
#include "engine.h"
#include "../base/asset_file.h"
#include "../base/log.h"

namespace eng {

bool ShaderSource::Load(const std::string& name) {
  if (IsImmutable()) {
    LOG << "Error: ShaderSource is immutable. Failed to load.";
    return false;;
  }

  std::string vertex_file_name = name;
  vertex_file_name += "_vertex.glsl";
  vertex_source_ = base::AssetFile::ReadWholeFile(vertex_file_name.c_str(),
      Engine::Get().GetRootPath().c_str(), NULL, true);
  if (!vertex_source_) {
    LOG << "Failed to read file: " << vertex_file_name;
    return false;
  }

  std::string fragment_file_name = name;
  fragment_file_name += "_fragment.glsl";
  fragment_source_ = base::AssetFile::ReadWholeFile(fragment_file_name.c_str(),
      Engine::Get().GetRootPath().c_str(), NULL, true);
  if (!fragment_source_) {
    LOG << "Failed to read file: " << fragment_file_name;
    return false;
  }

  return true;
}

}  // namespace eng
