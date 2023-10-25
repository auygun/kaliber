#include "engine/renderer/opengl/renderer_opengl.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <unordered_set>

#include "base/hash.h"
#include "base/log.h"
#include "base/vecmath.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

using namespace base;

namespace {

constexpr GLenum kGlPrimitive[eng::kPrimitive_Max] = {GL_TRIANGLES,
                                                      GL_TRIANGLE_STRIP};

constexpr GLenum kGlDataType[eng::kDataType_Max] = {
    GL_UNSIGNED_BYTE, GL_FLOAT,        GL_INT,
    GL_SHORT,         GL_UNSIGNED_INT, GL_UNSIGNED_SHORT};

constexpr GLboolean kGlNormalize[eng::kDataType_Max] = {
    GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};

const std::string kAttributeNames[eng::kAttribType_Max] = {
    "in_color", "in_normal", "in_position", "in_tex_coord"};

}  // namespace

namespace eng {

RendererOpenGL::RendererOpenGL(base::Closure context_lost_cb)
    : Renderer(context_lost_cb) {}

RendererOpenGL::~RendererOpenGL() {
  Shutdown();
  OnDestroy();
}

void RendererOpenGL::OnWindowResized(int width, int height) {
  screen_width_ = width;
  screen_height_ = height;
  glViewport(0, 0, screen_width_, screen_height_);
}

int RendererOpenGL::GetScreenWidth() const {
  return screen_width_;
}

int RendererOpenGL::GetScreenHeight() const {
  return screen_height_;
}

void RendererOpenGL::SetScissor(int x, int y, int width, int height) {
  glScissor(x, screen_height_ - y - height, width, height);
  glEnable(GL_SCISSOR_TEST);
}

void RendererOpenGL::ResetScissor() {
  glDisable(GL_SCISSOR_TEST);
}

uint64_t RendererOpenGL::CreateGeometry(std::unique_ptr<Mesh> mesh) {
  // Verify that we have a valid layout and get the total byte size per vertex.
  GLuint vertex_size = mesh->GetVertexSize();
  if (!vertex_size) {
    LOG(0) << "Invalid vertex layout";
    return 0;
  }

  GLuint vertex_array_id = 0;
  if (vertex_array_objects_) {
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);
  }

  // Create the vertex buffer and upload the data.
  GLuint vertex_buffer_id = 0;
  glGenBuffers(1, &vertex_buffer_id);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
  glBufferData(GL_ARRAY_BUFFER, mesh->num_vertices() * vertex_size,
               mesh->GetVertices(), GL_STATIC_DRAW);

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  std::vector<GeometryOpenGL::Element> vertex_layout;
  if (!SetupVertexLayout(mesh->vertex_description(), vertex_size,
                         vertex_array_objects_, vertex_layout)) {
    LOG(0) << "Invalid vertex layout";
    return 0;
  }

  // Create the index buffer and upload the data.
  GLuint index_buffer_id = 0;
  if (mesh->GetIndices()) {
    glGenBuffers(1, &index_buffer_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mesh->num_indices() * mesh->GetIndexSize(), mesh->GetIndices(),
                 GL_STATIC_DRAW);
  }

  if (vertex_array_id) {
    // De-activate the buffer again and we're done.
    glBindVertexArray(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  uint64_t resource_id = ++last_resource_id_;
  geometries_[resource_id] = {(GLsizei)mesh->num_vertices(),
                              (GLsizei)mesh->num_indices(),
                              kGlPrimitive[mesh->primitive()],
                              kGlDataType[mesh->index_description()],
                              vertex_layout,
                              vertex_size,
                              vertex_array_id,
                              vertex_buffer_id,
                              index_buffer_id};
  return resource_id;
}

void RendererOpenGL::DestroyGeometry(uint64_t resource_id) {
  auto it = geometries_.find(resource_id);
  if (it == geometries_.end())
    return;

  if (it->second.index_buffer_id)
    glDeleteBuffers(1, &(it->second.index_buffer_id));
  if (it->second.vertex_buffer_id)
    glDeleteBuffers(1, &(it->second.vertex_buffer_id));
  if (it->second.vertex_array_id)
    glDeleteVertexArrays(1, &(it->second.vertex_array_id));

  geometries_.erase(it);
}

void RendererOpenGL::Draw(uint64_t resource_id,
                          uint64_t num_indices,
                          uint64_t start_offset) {
  auto it = geometries_.find(resource_id);
  if (it == geometries_.end())
    return;

  if (num_indices == 0)
    num_indices = it->second.num_indices;

  // Set up the vertex data.
  if (it->second.vertex_array_id)
    glBindVertexArray(it->second.vertex_array_id);
  else {
    glBindBuffer(GL_ARRAY_BUFFER, it->second.vertex_buffer_id);
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)it->second.vertex_layout.size();
         ++attribute_index) {
      GeometryOpenGL::Element& e = it->second.vertex_layout[attribute_index];
      glEnableVertexAttribArray(attribute_index);
      glVertexAttribPointer(attribute_index, e.num_elements, e.type, GL_FALSE,
                            it->second.vertex_size,
                            (const GLvoid*)e.vertex_offset);
    }

    if (num_indices > 0)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.index_buffer_id);
  }

  // Draw the primitive.
  if (num_indices > 0)
    glDrawElements(it->second.primitive, num_indices, it->second.index_type,
                   (void*)(intptr_t)(start_offset * sizeof(unsigned short)));
  else
    glDrawArrays(it->second.primitive, 0, it->second.num_vertices);

  // Clean up states.
  if (it->second.vertex_array_id)
    glBindVertexArray(0);
  else {
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)it->second.vertex_layout.size();
         ++attribute_index)
      glDisableVertexAttribArray(attribute_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

uint64_t RendererOpenGL::CreateTexture() {
  GLuint gl_id = 0;
  glGenTextures(1, &gl_id);
  glBindTexture(GL_TEXTURE_2D, gl_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  uint64_t resource_id = ++last_resource_id_;
  textures_[resource_id] = gl_id;
  return resource_id;
}

void RendererOpenGL::UpdateTexture(uint64_t resource_id,
                                   std::unique_ptr<Image> image) {
  auto it = textures_.find(resource_id);
  if (it == textures_.end())
    return;

  glBindTexture(GL_TEXTURE_2D, it->second);
  if (image->IsCompressed()) {
    GLenum format = 0;
    switch (image->GetFormat()) {
      case Image::kDXT1:
        format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        break;
      case Image::kDXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
      case Image::kETC1:
        format = GL_ETC1_RGB8_OES;
        break;
#if defined(__ANDROID__)
      case Image::kATC:
        format = GL_ATC_RGB_AMD;
        break;
      case Image::kATCIA:
        format = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        break;
#endif
      default:
        NOTREACHED() << "- Unhandled texure format: " << image->GetFormat();
    }

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, image->GetWidth(),
                           image->GetHeight(), 0, image->GetSize(),
                           image->GetBuffer());

    // On some devices the first glCompressedTexImage2D call after context-lost
    // returns GL_INVALID_VALUE for some reason.
    GLenum err = glGetError();
    if (err == GL_INVALID_VALUE) {
      glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, image->GetWidth(),
                             image->GetHeight(), 0, image->GetSize(),
                             image->GetBuffer());
      err = glGetError();
    }

    if (err != GL_NO_ERROR)
      LOG(0) << "GL ERROR after glCompressedTexImage2D: " << (int)err;
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->GetWidth(),
                 image->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image->GetBuffer());
  }
}

void RendererOpenGL::DestroyTexture(uint64_t resource_id) {
  auto it = textures_.find(resource_id);
  if (it == textures_.end())
    return;

  glDeleteTextures(1, &(it->second));
  textures_.erase(it);
}

void RendererOpenGL::ActivateTexture(uint64_t resource_id,
                                     uint64_t texture_unit) {
  if (texture_unit >= kMaxTextureUnits) {
    DLOG(0) << "Invalid texture unit " << texture_unit;
    return;
  }

  auto it = textures_.find(resource_id);
  if (it == textures_.end()) {
    return;
  }

  if (it->second != active_texture_id_[texture_unit]) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, it->second);
    active_texture_id_[texture_unit] = it->second;
  }
}

uint64_t RendererOpenGL::CreateShader(
    std::unique_ptr<ShaderSource> source,
    const VertexDescription& vertex_description,
    Primitive primitive,
    bool enable_depth_test) {
  GLuint vertex_shader =
      CreateShader(source->GetVertexSource(), GL_VERTEX_SHADER);
  if (!vertex_shader)
    return 0;

  GLuint fragment_shader =
      CreateShader(source->GetFragmentSource(), GL_FRAGMENT_SHADER);
  if (!fragment_shader)
    return 0;

  GLuint id = glCreateProgram();
  if (id) {
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    if (!BindAttributeLocation(id, vertex_description)) {
      glDeleteProgram(id);
      return 0;
    }

    glLinkProgram(id);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint length = 0;
      glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
      if (length > 0) {
        char* buffer = (char*)malloc(length);
        if (buffer) {
          glGetProgramInfoLog(id, length, NULL, buffer);
          LOG(0) << "Could not link program:\n" << buffer;
          free(buffer);
        }
      }
      glDeleteProgram(id);
      return 0;
    }
  }

  uint64_t resource_id = ++last_resource_id_;
  shaders_[resource_id] = {id, {}, enable_depth_test};
  return resource_id;
}

void RendererOpenGL::DestroyShader(uint64_t resource_id) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  glDeleteProgram(it->second.id);
  shaders_.erase(it);
}

void RendererOpenGL::ActivateShader(uint64_t resource_id) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  if (it->second.id != active_shader_id_) {
    glUseProgram(it->second.id);
    active_shader_id_ = it->second.id;
    if (it->second.enable_depth_test)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
  }
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                const base::Vector2f& val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniform2fv(index, 1, val.GetData());
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                const base::Vector3f& val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniform3fv(index, 1, val.GetData());
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                const base::Vector4f& val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniform4fv(index, 1, val.GetData());
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                const base::Matrix4f& val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniformMatrix4fv(index, 1, GL_FALSE, val.GetData());
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                float val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniform1f(index, val);
}

void RendererOpenGL::SetUniform(uint64_t resource_id,
                                const std::string& name,
                                int val) {
  auto it = shaders_.find(resource_id);
  if (it == shaders_.end())
    return;

  GLint index = GetUniformLocation(it->second.id, name, it->second.uniforms);
  if (index >= 0)
    glUniform1i(index, val);
}

void RendererOpenGL::PrepareForDrawing() {
  glDisable(GL_SCISSOR_TEST);
}

void RendererOpenGL::ContextLost() {
  LOG(0) << "Context lost.";

  DestroyAllResources();
  context_lost_cb_();
}

size_t RendererOpenGL::GetAndResetFPS() {
  int ret = fps_;
  fps_ = 0;
  return ret;
}

bool RendererOpenGL::InitCommon() {
  // Get information about the currently active context.
  const char* renderer =
      reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  LOG(0) << "OpenGL:";
  LOG(0) << "  vendor:         " << (const char*)glGetString(GL_VENDOR);
  LOG(0) << "  renderer:       " << renderer;
  LOG(0) << "  version:        " << version;
  LOG(0) << "  shader version: "
         << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  LOG(0) << "Screen size: " << screen_width_ << ", " << screen_height_;

  // Setup extensions.
  std::stringstream stream((const char*)glGetString(GL_EXTENSIONS));
  std::string token;
  std::unordered_set<std::string> extensions;
  while (std::getline(stream, token, ' '))
    extensions.insert(token);

#if 0
  LOG(0) << "  extensions:";
  for (auto& ext : extensions)
    LOG(0) << "    " << ext.c_str();
#endif

  // Check for supported texture compression extensions.
  if (extensions.find("GL_OES_compressed_ETC1_RGB8_texture") !=
      extensions.end())
    texture_compression_.etc1 = true;
  if (extensions.find("GL_EXT_texture_compression_dxt1") != extensions.end())
    texture_compression_.dxt1 = true;
  if (extensions.find("GL_EXT_texture_compression_latc") != extensions.end())
    texture_compression_.latc = true;
  if (extensions.find("GL_EXT_texture_compression_s3tc") != extensions.end())
    texture_compression_.s3tc = true;
  if (extensions.find("GL_IMG_texture_compression_pvrtc") != extensions.end())
    texture_compression_.pvrtc = true;
  if (extensions.find("GL_AMD_compressed_ATC_texture") != extensions.end() ||
      extensions.find("GL_ATI_texture_compression_atitc") != extensions.end())
    texture_compression_.atc = true;

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end() ||
      extensions.find("GL_ARB_vertex_array_object") != extensions.end()) {
    // This extension seems to be broken on older PowerVR drivers.
    if (!strstr(renderer, "PowerVR SGX 53") &&
        !strstr(renderer, "PowerVR SGX 54") &&
        !strstr(renderer, "Android Emulator")) {
      vertex_array_objects_ = true;
    }
  }

  if (extensions.count("GL_ARB_texture_non_power_of_two") ||
      extensions.count("GL_OES_texture_npot")) {
    npot_ = true;
  }

  // Ancient hardware is not supported.
  if (!npot_)
    LOG(0) << "NPOT not supported.";

  if (vertex_array_objects_)
    LOG(0) << "Supports Vertex Array Objects.";

  LOG(0) << "TextureCompression:";
  LOG(0) << "  atc:   " << texture_compression_.atc;
  LOG(0) << "  dxt1:  " << texture_compression_.dxt1;
  LOG(0) << "  etc1:  " << texture_compression_.etc1;
  LOG(0) << "  s3tc:  " << texture_compression_.s3tc;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0, 0, 0, 1);

  is_initialized_ = true;

  return true;
}

void RendererOpenGL::DestroyAllResources() {
  std::vector<uint64_t> resource_ids;
  for (auto& r : geometries_)
    resource_ids.push_back(r.first);
  for (auto& r : resource_ids)
    DestroyGeometry(r);

  resource_ids.clear();
  for (auto& r : shaders_)
    resource_ids.push_back(r.first);
  for (auto& r : resource_ids)
    DestroyShader(r);

  resource_ids.clear();
  for (auto& r : textures_)
    resource_ids.push_back(r.first);
  for (auto& r : resource_ids)
    DestroyTexture(r);

  DCHECK(geometries_.size() == 0);
  DCHECK(shaders_.size() == 0);
  DCHECK(textures_.size() == 0);
}

bool RendererOpenGL::SetupVertexLayout(
    const VertexDescription& vd,
    GLuint vertex_size,
    bool use_vao,
    std::vector<GeometryOpenGL::Element>& vertex_layout) {
  GLuint attribute_index = 0;
  size_t vertex_offset = 0;

  for (auto& attr : vd) {
    // There's a limitation of 16 attributes in OpenGL ES 2.0
    if (attribute_index >= 16)
      return false;

    auto [attrib_type, data_type, num_elements, type_size] = attr;

    // The data type is needed, the most common ones are supported.
    GLenum type = kGlDataType[data_type];

    // We got all we need to define this attribute.
    if (use_vao) {
      // This will be saved into the vertex array object.
      glEnableVertexAttribArray(attribute_index);
      glVertexAttribPointer(attribute_index, num_elements, type,
                            kGlNormalize[data_type], vertex_size,
                            (const GLvoid*)vertex_offset);
    } else {
      // Need to keep this information for when rendering.
      GeometryOpenGL::Element element;
      element.num_elements = num_elements;
      element.type = type;
      element.vertex_offset = vertex_offset;
      vertex_layout.push_back(element);
    }

    // Move on to the next attribute.
    ++attribute_index;
    vertex_offset += num_elements * type_size;
  }
  return true;
}

GLuint RendererOpenGL::CreateShader(const char* source, GLenum type) {
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
        char* buffer = (char*)malloc(length);
        if (buffer) {
          glGetShaderInfoLog(shader, length, NULL, buffer);
          LOG(0) << "Could not compile shader " << type << ":\n" << buffer;
          free(buffer);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }
  return shader;
}

bool RendererOpenGL::BindAttributeLocation(GLuint id,
                                           const VertexDescription& vd) {
  int current = 0;
  int tex_coord = 0;

  for (auto& attr : vd) {
    AttribType attrib_type = std::get<0>(attr);
    std::string attrib_name = kAttributeNames[attrib_type];
    if (attrib_type == kAttribType_TexCoord)
      attrib_name += std::to_string(tex_coord++);
    glBindAttribLocation(id, current++, attrib_name.c_str());
  }
  return current > 0;
}

GLint RendererOpenGL::GetUniformLocation(
    GLuint id,
    const std::string& name,
    std::vector<std::pair<size_t, GLuint>>& uniforms) {
  // Check if we've encountered this uniform before.
  auto hash = KR2Hash(name);
  auto it = std::find_if(uniforms.begin(), uniforms.end(),
                         [&](auto& r) { return hash == std::get<0>(r); });
  GLint index;
  if (it != uniforms.end()) {
    // Yes, we already have the mapping.
    index = std::get<1>(*it);
  } else {
    // No, ask the driver for the mapping and save it.
    index = glGetUniformLocation(id, name.c_str());
    if (index >= 0) {
      DCHECK(std::find_if(uniforms.begin(), uniforms.end(),
                          [&](auto& r) { return hash == std::get<0>(r); }) ==
             uniforms.end())
          << "Hash collision";
      uniforms.emplace_back(std::make_pair(hash, index));
    } else {
      LOG(0) << "Cannot find uniform " << name.c_str() << " (shader: " << id
             << ")";
    }
  }
  return index;
}

}  // namespace eng
