#include "../../base/file.h"
#include "../../base/log.h"
#include "shader.h"
#include <stdlib.h>
#include <string>
#include <string.h>

namespace engine {

Shader::Shader()
  : id(0) {
}

Shader::~Shader() {
  Destroy();
}

bool Shader::Create(const char *name, const char *vertexDescription) {
  Destroy();
  return CreateProgram(name, vertexDescription);
}

void Shader::Destroy() {
  if (id) {
    glDeleteProgram(id);
    id = 0;
  }
  uniforms.clear();
}

void Shader::Activate() {
  glUseProgram(id);
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

bool Shader::CreateProgram(const char *name, const char *vertexDescription) {
  char *vertexSource = NULL;
  char *fragmentSource = NULL;
  bool result = false;

  do {
    std::string vertexFileName = name;
    vertexFileName += "_vertex.glsl";
    char *vertexSource = File::ReadWholeFile(vertexFileName.c_str(), NULL, true);
    if (!vertexSource)
      break;

    std::string fragmentFileName = name;
    fragmentFileName += "_fragment.glsl";
    char *fragmentSource = File::ReadWholeFile(fragmentFileName.c_str(), NULL, true);
    if (!fragmentSource)
      break;

    result = CreateProgram(vertexSource, fragmentSource, vertexDescription);
  }
  while (false);

  delete [] vertexSource;
  delete [] fragmentSource;

  return result;
}

bool Shader::CreateProgram(const char *vertexSource, const char *fragmentSource,
                           const char *vertexDescription) {
  GLuint vertexShader = CreateShader(vertexSource, GL_VERTEX_SHADER);
  if (!vertexShader)
    return false;

  GLuint fragmentShader = CreateShader(fragmentSource, GL_FRAGMENT_SHADER);
  if (!fragmentShader)
    return false;

  id = glCreateProgram();
  if (id) {
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    if (!BindAttributeLocation(vertexDescription))
      return false;

    glLinkProgram(id);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint length = 0;
      glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
      if (length > 0) {
        char *buffer = (char *)malloc(length);
        if (buffer) {
          glGetProgramInfoLog(id, length, NULL, buffer);
          LOG("Could not link program:\n%s\n", buffer);
          free(buffer);
        }
      }
      glDeleteProgram(id);
      id = 0;
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

bool Shader::BindAttributeLocation(const char *vertexDescription) {
  int current = 0,
      texCoord = 0;

  // Parse the description.
  const char kLayoutDelimiters[] = ";/ \t";
  char buffer[32];
  strcpy(buffer, vertexDescription);
  char *token = strtok(buffer, kLayoutDelimiters);

  char texCoordBuffer[32];

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    switch (token[0]) {
    case 'c': glBindAttribLocation(id, current++, "inColor");     break;
    case 'n': glBindAttribLocation(id, current++, "inNormal");    break;
    case 'p': glBindAttribLocation(id, current++, "inPosition");  break;

    case 't':
      sprintf(texCoordBuffer, "inTexCoord%d", texCoord++);
      glBindAttribLocation(id, current++, texCoordBuffer);
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
  UniformMap::iterator i = uniforms.find(name);
  GLint index;
  if (i != uniforms.end()) {
    // Yes, we already have the mapping.
    index = i->second;
  } else {
    // No, ask the driver for the mapping and save it.
    index = glGetUniformLocation(id, name.c_str());
    uniforms[name] = index;
  }
  return index;
}

} // namespace engine
