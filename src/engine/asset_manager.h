#ifndef ENGINE_ASSET_MANAGER_H
#define ENGINE_ASSET_MANAGER_H

#include <memory>
#include <string>
#include <vector>

#include "engine/model.h"

namespace eng {

class AssetManager {
 public:
  AssetManager();
  ~AssetManager();

  // Loads a GLTF model and returns its ID (index).
  uint32_t LoadGLTF(const std::string& file_name, uint64_t shader_id);

  // Loads an OBJ model and returns its ID (index).
  uint32_t LoadObj(const std::string& file_name,
                   uint64_t shader_id,
                   const std::string& mtl_file_name = "",
                   const std::vector<std::string>& texture_file_names = {});

  // Creates a procedural sphere mesh and returns its ID (index).
  uint32_t CreateSphere(uint64_t shader_id,
                        size_t rings,
                        size_t sectors,
                        const std::vector<std::string>& texture_file_names);

  // Returns a pointer to the model given its ID.
  Model* GetModel(uint32_t index);

 private:
  std::vector<std::unique_ptr<Model>> models_;
};

}  // namespace eng

#endif  // ENGINE_ASSET_MANAGER_H
