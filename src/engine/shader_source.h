#ifndef SHADER_CODE_H
#define SHADER_CODE_H

#include <memory>
#include <string>

namespace eng {

class ShaderSource {
 public:
  ShaderSource() = default;
  ~ShaderSource() = default;

  bool Load(const std::string& name);

  const char* GetVertexSource() const { return vertex_source_.get(); }
  const char* GetFragmentSource() const { return fragment_source_.get(); }

  size_t vertex_source_size() const { return vertex_source_size_; }
  size_t fragment_source_size() const { return fragment_source_size_; }

  const std::string& name() const { return name_; }

 private:
  std::string name_;

  std::unique_ptr<char[]> vertex_source_;
  std::unique_ptr<char[]> fragment_source_;

  size_t vertex_source_size_ = 0;
  size_t fragment_source_size_ = 0;

  size_t LoadInternal(const std::string& name,
                      std::unique_ptr<char[]>& dst,
                      const char* inject,
                      size_t inject_len);
};

}  // namespace eng

#endif  // SHADER_CODE_H
