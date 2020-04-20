#ifndef SHADER_H
#define SHADER_H

#include "../../base/vecmath.h"
#include "opengl.h"
#include <unordered_map>
#include <string>

namespace engine {

class Shader {
public:
  Shader();
  ~Shader();

  bool Create(const char *name, const char *vertex_description);
  void Destroy();
  void Activate();

  void SetUniform(const std::string &name, const Vector2 &v);
  void SetUniform(const std::string &name, const Vector3 &v);
  void SetUniform(const std::string &name, int i);


private:
  typedef std::unordered_map<std::string, GLuint> UniformMap;

  GLuint      id_;
  UniformMap  uniforms_;


  bool CreateProgram(const char *name, const char *vertex_description);
  bool CreateProgram(const char *vertex_source, const char *fragment_source,
                     const char *vertex_description);
  GLuint CreateShader(const char *source, GLenum type);
  bool BindAttributeLocation(const char *vertex_description);
  GLint GetUniformLocation(const std::string &name);
};

} // namespace engine

#endif // SHADER_H
