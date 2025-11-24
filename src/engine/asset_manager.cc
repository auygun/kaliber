#include "engine/asset_manager.h"

#include "base/log.h"
#include "base/vecmath.h"
#include "engine/engine.h"
#include "engine/renderer/renderer.h"

namespace eng {

AssetManager::AssetManager() = default;
AssetManager::~AssetManager() = default;

uint32_t AssetManager::LoadGLTF(const std::string& file_name,
                                uint64_t shader_id) {
  auto model = std::make_unique<Model>();
  if (model->LoadGLTF(Engine::Get().GetRenderer(), shader_id, file_name)) {
    models_.push_back(std::move(model));
    return models_.size() - 1;
  }
  return 0;  // Return 0 or invalid ID on failure
}

uint32_t AssetManager::LoadObj(
    const std::string& file_name,
    uint64_t shader_id,
    const std::string& mtl_file_name,
    const std::vector<std::string>& texture_file_names) {
  auto model = std::make_unique<Model>();
  if (model->LoadObj(Engine::Get().GetRenderer(), shader_id, file_name,
                     mtl_file_name, texture_file_names)) {
    models_.push_back(std::move(model));
    return models_.size() - 1;
  }
  return 0;
}

uint32_t AssetManager::CreateSphere(
    uint64_t shader_id,
    size_t rings,
    size_t sectors,
    const std::vector<std::string>& texture_file_names) {
  std::vector<Model::Vertex> vertices;
  std::vector<uint32_t> indices;

  float const R = 1. / (float)(rings - 1);
  float const S = 1. / (float)(sectors - 1);

  for (size_t r = 0; r < rings; ++r) {
    for (size_t s = 0; s < sectors; ++s) {
      float y = sin(-base::PIHALFf + base::PIf * r * R);
      float x = cos(2 * base::PIf * s * S) * sin(base::PIf * r * R);
      float z = sin(2 * base::PIf * s * S) * sin(base::PIf * r * R);
      float u = s * S;
      float v = r * R;

      Model::Vertex vert{};
      vert.position = {x, y, z};
      vert.normal = {x, y, z};
      vert.uv[0] = u;
      vert.uv[1] = v;
      vertices.push_back(vert);

      if (r < rings - 1) {
        size_t curRow = r * sectors;
        size_t nextRow = (r + 1) * sectors;
        size_t nextS = (s + 1) % sectors;

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));
        indices.push_back((uint32_t)(curRow + nextS));
      }
    }
  }

  auto model = std::make_unique<Model>();
  model->CreateMesh(Engine::Get().GetRenderer(), shader_id, std::move(vertices),
                    std::move(indices), texture_file_names);
  models_.push_back(std::move(model));
  return models_.size() - 1;
}

Model* AssetManager::GetModel(uint32_t index) {
  if (index < models_.size()) {
    return models_[index].get();
  }
  LOG(0) << "Invalid model index: " << index;
  return nullptr;
}

}  // namespace eng
