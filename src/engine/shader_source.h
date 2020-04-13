#ifndef SHADER_CODE_H
#define SHADER_CODE_H

#include <memory>
#include <string>
#include "asset.h"

namespace eng {

class ShaderSource : public Asset {
 public:
  ShaderSource() = default;
  ~ShaderSource() override = default;

  bool Load(const std::string& name) override;

  const char* GetVertexSource() const { return vertex_source_.get(); }
  const char* GetFragmentSource() const { return fragment_source_.get(); }

 private:
  std::unique_ptr<char[]> vertex_source_;
  std::unique_ptr<char[]> fragment_source_;

  std::unique_ptr<char[]> Preprocess(char* source, size_t length);
};

}  // namespace eng

#endif  // SHADER_CODE_H
