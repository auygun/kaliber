#ifndef SHADER_CODE_H
#define SHADER_CODE_H

#include "asset.h"
#include <string>
#include <memory>

namespace eng {

class ShaderCode : public Asset {
 public:
  ShaderCode() = default;
  ~ShaderCode() override = default;

  bool Load(const std::string& name);

  const char* GetVertexCode() const { return vertex_code_.get(); }
  const char* GetFragmentCode() const { return fragment_code_.get(); }

 private:
  std::unique_ptr<char[]> vertex_code_;
  std::unique_ptr<char[]> fragment_code_;
};

}  // namespace eng

#endif  // SHADER_CODE_H
