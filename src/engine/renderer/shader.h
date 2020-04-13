#ifndef SHADER_H
#define SHADER_H

#include "../vecmath.h"
#include "opengl.h"
#include <map>
#include <string>

namespace engine {

class Shader {
public:
  Shader();
  ~Shader();

  bool Create(const char *name, const char *vertexDescription);
  void Destroy();
  void Activate();

  void SetUniform(const std::string &name, const Vector2 &v);
  void SetUniform(const std::string &name, const Vector3 &v);
  void SetUniform(const std::string &name, int i);


private:
  typedef std::map<std::string, GLuint> UniformMap;

  GLuint      id;
  UniformMap  uniforms;


  bool CreateProgram(const char *name, const char *vertexDescription);
  bool CreateProgram(const char *vertexSource, const char *fragmentSource,
                     const char *vertexDescription);
  GLuint CreateShader(const char *source, GLenum type);
  bool BindAttributeLocation(const char *vertexDescription);
  GLint GetUniformLocation(const std::string &name);
};

} // namespace engine

#endif // SHADER_H
