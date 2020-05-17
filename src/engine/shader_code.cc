#include "shader_code.h"
#include "engine.h"
#include "../base/asset_loader.h"
#include "../base/log.h"

namespace eng {

bool ShaderCode::Load(const std::string& name) {
  if (IsImmutable()) {
    LOG << "Error: ShaderCode is immutable. Failed to load.";
    return false;;
  }

  std::string vertex_file_name = "shaders/";
  vertex_file_name += name;
  vertex_file_name += "_vertex.glsl";
  vertex_code_ = AssetLoader::ReadWholeFile(vertex_file_name.c_str(),
      Engine::Get().GetRootPath().c_str(), NULL, true);
  if (!vertex_code_)
    return false;

  std::string fragment_file_name = "shaders/";
  fragment_file_name += name;
  fragment_file_name += "_fragment.glsl";
  fragment_code_ = AssetLoader::ReadWholeFile(fragment_file_name.c_str(),
      Engine::Get().GetRootPath().c_str(), NULL, true);
  if (!fragment_code_)
    return false;

  return true;
}

}  // namespace eng
