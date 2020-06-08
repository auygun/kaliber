#ifndef SHADER_CODE_H
#define SHADER_CODE_H

#include <string>
#include "asset.h"

namespace eng {

class ShaderSource : public Asset {
 public:
  ShaderSource() = default;
  ~ShaderSource() override = default;

  bool Load(const std::string& name) override;

  const std::string& GetVertexSource() const { return vertex_source_; }
  const std::string& GetFragmentSource() const { return fragment_source_; }

 private:
  std::string vertex_source_;
  std::string fragment_source_;
};

}  // namespace eng

#endif  // SHADER_CODE_H
