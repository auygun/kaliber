#include "../../base/file.h"
#include "../../base/log.h"
#include "shader.h"
#include <stdlib.h>
#include <string>
#include <string.h>

namespace engine {

Shader::Shader()
  : id_(0) {
}

Shader::~Shader() {
  Destroy();
}

bool Shader::Create(const char *name, const char *vertex_description) {
  Destroy();
  return CreateProgram(name, vertex_description);
}

void Shader::Destroy() {
  if (id_) {
    glDeleteProgram(id_);
    id_ = 0;
  }
  uniforms_.clear();
}

void Shader::Activate() {
  glUseProgram(id_);
}

void Shader::SetUniform(const std::string &name, const Vector2 &v) {
  // Update the value if it's valid.
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform2fv(index, 1, v.GetData());
}

void Shader::SetUniform(const std::string &name, const Vector3 &v) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform3fv(index, 1, v.GetData());
}

void Shader::SetUniform(const std::string &name, int i) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform1i(index, i);
}

bool Shader::CreateProgram(const char *name, const char *vertex_description) {
  char *vertex_source = NULL;
  char *fragment_source = NULL;
  bool result = false;

  do {
    std::string vertex_file_name = name;
    vertex_file_name += "_vertex.glsl";
    char *vertex_source = File::ReadWholeFile(vertex_file_name.c_str(), NULL, true);
    if (!vertex_source)
      break;

    std::string fragment_file_name = name;
    fragment_file_name += "_fragment.glsl";
    char *fragment_source = File::ReadWholeFile(fragment_file_name.c_str(), NULL, true);
    if (!fragment_source)
      break;

    result = CreateProgram(vertex_source, fragment_source, vertex_description);
  }
  while (false);

  delete [] vertex_source;
  delete [] fragment_source;

  return result;
}

bool Shader::CreateProgram(const char *vertex_source, const char *fragment_source,
                           const char *vertex_description) {
  GLuint vertex_shader = CreateShader(vertex_source, GL_VERTEX_SHADER);
  if (!vertex_shader)
    return false;

  GLuint fragment_shader = CreateShader(fragment_source, GL_FRAGMENT_SHADER);
  if (!fragment_shader)
    return false;

  id_ = glCreateProgram();
  if (id_) {
    glAttachShader(id_, vertex_shader);
    glAttachShader(id_, fragment_shader);
    if (!BindAttributeLocation(vertex_description))
      return false;

    glLinkProgram(id_);
    GLint link_status = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
      GLint length = 0;
      glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
      if (length > 0) {
        char *buffer = (char *)malloc(length);
        if (buffer) {
          glGetProgramInfoLog(id_, length, NULL, buffer);
          LOG("Could not link program:\n%s\n", buffer);
          free(buffer);
        }
      }
      glDeleteProgram(id_);
      id_ = 0;
      return false;
    }
  }

  return true;
}

GLuint Shader::CreateShader(const char *source, GLenum type) {
  GLuint shader = glCreateShader(type);
  if (shader) {
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      GLint length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
      if (length) {
        char *buffer = (char *)malloc(length);
        if (buffer) {
          glGetShaderInfoLog(shader, length, NULL, buffer);
          LOG("Could not compile shader %d:\n%s\n", type, buffer);
          free(buffer);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }

  return shader;
}

bool Shader::BindAttributeLocation(const char *vertex_description) {
  int current = 0,
      tex_coord = 0;

  // Parse the description.
  const char kLayoutDelimiters[] = ";/ \t";
  char buffer[32];
  strcpy(buffer, vertex_description);
  char *token = strtok(buffer, kLayoutDelimiters);

  char tex_coord_buffer[32];

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    switch (token[0]) {
    case 'c': glBindAttribLocation(id_, current++, "inColor");     break;
    case 'n': glBindAttribLocation(id_, current++, "inNormal");    break;
    case 'p': glBindAttribLocation(id_, current++, "inPosition");  break;

    case 't':
      sprintf(tex_coord_buffer, "inTexCoord%d", tex_coord++);
      glBindAttribLocation(id_, current++, tex_coord_buffer);
      break;

    default:
      LOG("Unknown attribute: %s\n", token);
      return false;
    }

    token = strtok(NULL, kLayoutDelimiters);
  }

  // We need at least one position attribute.
  return current > 0;
}

GLint Shader::GetUniformLocation(const std::string &name) {
  // Check if we've encountered this uniform before.
  UniformMap::iterator i = uniforms_.find(name);
  GLint index;
  if (i != uniforms_.end()) {
    // Yes, we already have the mapping.
    index = i->second;
  } else {
    // No, ask the driver for the mapping and save it.
    index = glGetUniformLocation(id_, name.c_str());
    uniforms_[name] = index;
  }
  return index;
}

} // namespace engine
