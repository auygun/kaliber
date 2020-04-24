#include "renderer.h"
#include <sstream>
#include <cassert>
#include <cstring>
#include "../../base/log.h"
#include "render_command.h"

namespace {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char kLayoutDelimiter[] = ";/ \t";

GLuint GetVertexSize(const std::string &vertexDescription) {
  GLuint size = 0;

  // Parse the description.
  char buffer[32];
  strcpy(buffer, vertexDescription.c_str());
  char *token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    size_t numElements = token[1] - '1' + 1;
    if (numElements < 1 || numElements > 4)
      return 0;

    // The data type is needed, the most common ones are supported.
    size_t typeSize;
    switch (token[2]) {
    case 'b': typeSize = sizeof(GLbyte);    break;
    case 'f': typeSize = sizeof(GLfloat);   break;
    case 'i': typeSize = sizeof(GLint);     break;
    case 's': typeSize = sizeof(GLshort);   break;
    case 'u': typeSize = sizeof(GLuint);    break;
    case 'w': typeSize = sizeof(GLushort);  break;
    default:  return 0;
    }

    size += numElements * typeSize;

    token = strtok(NULL, kLayoutDelimiter);
  }

  return size;
}

unsigned GetIndexSize(GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE:  return sizeof(GLbyte);
  case GL_UNSIGNED_SHORT: return sizeof(GLushort);
  case GL_UNSIGNED_INT:   return sizeof(GLuint);
  default:                return 0;
  }
}

} // namespace

namespace engine {

Renderer::Renderer() {}

Renderer::~Renderer() {
  TerminateWorker();
}

bool Renderer::StartWorker() {
#ifdef THREADED_RENDERING
  LOG << "Strating render thread.";
  std::promise<bool> promise;
  std::future<bool> future = promise.get_future();
  worker_thread_ = std::thread(&Renderer::WorkerMain, this, std::move(promise));
  return future.get();
#else
  LOG << "Single threaded rendering.";
  return Init();
#endif // THREADED_RENDERING
}

void Renderer::TerminateWorker() {
#ifdef THREADED_RENDERING
  // Notify worker thread and wait for it to terminate.
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    if (terminate_worker_)
      return;
    terminate_worker_ = true;
  }
  cv_.notify_one();
  LOG << "Terminating render thread";
  worker_thread_.join();
#else
  Shutdown();
#endif // THREADED_RENDERING
}

void Renderer::EnterDrawStage() {
  draw_stage_ = true;
}

void Renderer::ExitDrawStage() {
  draw_stage_ = false;
}

void Renderer::EnqueueCommand(std::unique_ptr<RenderCommand> cmd) {
#ifdef THREADED_RENDERING
  if (!draw_stage_) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    global_commands_.push_back(std::move(cmd));
    cv_.notify_one();
    return;
  }
  bool new_frame = cmd->cmd_id == HASH("CmdPresent");
  draw_commands_[1].push_back(std::move(cmd));
  if (new_frame) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      draw_commands_[0].swap(draw_commands_[1]);
      cv_.notify_one();
    }
#if 0
    int discarded = (int)draw_commands_[1].size();
    if (discarded)
      LOG << "Discarding " << discarded << " draw commands.";
#endif
    draw_commands_[1].clear();
  }
#else
  ProcessCommand(cmd.get());
#endif // THREADED_RENDERING
}

#ifdef THREADED_RENDERING

void Renderer::WorkerMain(std::promise<bool> promise) {
  promise.set_value(Init());

  std::deque<std::unique_ptr<RenderCommand>> cq[2];
  for(;;) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      cv_.wait(scoped_lock, [&]()->bool {
        return !global_commands_.empty() || !draw_commands_[0].empty() ||
            terminate_worker_;
      });
      if (terminate_worker_) {
        Shutdown();
        return;
      }
      cq[0].swap(global_commands_);
      cq[1].swap(draw_commands_[0]);
    }

#if 0
    LOG << "qlobal queue size: " << (int)cq[0].size();
    LOG << "draw queue size: " << (int)cq[1].size();
#endif

    while (!cq[0].empty()) {
      std::unique_ptr<RenderCommand> cmd;
      cmd.swap(cq[0].front());
      cq[0].pop_front();
      ProcessCommand(cmd.get());
    }
    while (!cq[1].empty()) {
      std::unique_ptr<RenderCommand> cmd;
      cmd.swap(cq[1].front());
      cq[1].pop_front();
      ProcessCommand(cmd.get());
    }
  }
}

#endif // THREADED_RENDERING

void Renderer::ProcessCommand(RenderCommand* cmd) {
#if 0
  LOG << "Processing command: " << cmd->cmd_name.c_str();
#endif

  switch(cmd->cmd_id) {
  case HASH("CmdEableBlend"):
    HandleCmdEnableBlend(cmd);
    break;
  case HASH("CmdClear"):
    HandleCmdClear(cmd);
    break;
  case HASH("CmdPresent"):
    HandleCmdPresent(cmd);
    break;
  case HASH("CmdCreateTexture"):
    HandleCmdCreateTexture(cmd);
    break;
  case HASH("CmdDestoryTexture"):
    HandleCmdDestoryTexture(cmd);
    break;
  case HASH("CmdActivateTexture"):
    HandleCmdActivateTexture(cmd);
    break;
  case HASH("CmdCreateGeometry"):
    HandleCmdCreateGeometry(cmd);
    break;
  case HASH("CmdDestroyGeometry"):
    HandleCmdDestroyGeometry(cmd);
    break;
  case HASH("CmdDrawGeometry"):
    HandleCmdDrawGeometry(cmd);
    break;
  case HASH("CmdCreateShader"):
    HandleCmdCreateShader(cmd);
    break;
  case HASH("CmdDestroyShader"):
    HandleCmdDestroyShader(cmd);
    break;
  case HASH("CmdActivateShader"):
    HandleCmdActivateShader(cmd);
    break;
  case HASH("CmdSetUniformVec2"):
    HandleCmdSetUniformVec2(cmd);
    break;
  case HASH("CmdSetUniformVec3"):
    HandleCmdSetUniformVec3(cmd);
    break;
  case HASH("CmdSetUniformInt"):
    HandleCmdSetUniformInt(cmd);
    break;
  default:
    // assert(false);
    break;
  }
}


void Renderer::HandleCmdEnableBlend(RenderCommand* cmd) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::HandleCmdClear(RenderCommand* cmd) {
  auto *c = static_cast<CmdClear*>(cmd);
  glClearColor(c->rgba[0], c->rgba[1], c->rgba[2], c->rgba[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::HandleCmdCreateTexture(RenderCommand* cmd) {
  auto *c = static_cast<CmdCreateTexture*>(cmd);
  auto it = texture_map_.find(c->id);
  bool new_texture = it == texture_map_.end();

  GLuint gl_id = 0;
  if (new_texture)
    glGenTextures(1, &gl_id);
  else
    gl_id = it->second;

  // TODO: move to a separate update function.
  glBindTexture(GL_TEXTURE_2D, gl_id);
  if (c->image->IsCompressed()) {
    GLenum format;
    switch (c->image->GetFormat()) {
    case Image::kDXT1:  format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;    break;
    case Image::kDXT5:  format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;   break;
    case Image::kETC1:  format = GL_ETC1_RGB8_OES;                   break;
#if defined(__ANDROID__)
    case Image::kATC:   format = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD; break;
#endif
    default:
      assert(false);
      return;
    }

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format,
                           c->image->GetWidth(), c->image->GetHeight(), 0,
                           c->image->GetSize(), c->image->GetBuffer());

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      LOG << "GL ERROR after glCompressedTexImage2D: " << (int)err;
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c->image->GetWidth(), c->image->GetHeight(),
                   0, GL_RGBA, GL_UNSIGNED_BYTE, c->image->GetBuffer());
  }

  if (new_texture) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  texture_map_[c->id] = gl_id;
  // TODO: error handling.
}

void Renderer::HandleCmdDestoryTexture(RenderCommand* cmd) {
  auto *c = static_cast<CmdDestoryTexture*>(cmd);
  auto it = texture_map_.find(c->id);
  if (it != texture_map_.end()) {
    glDeleteTextures(1, &(it->second));
    texture_map_.erase(it);
  }
  // TODO: error handling
}

void Renderer::HandleCmdActivateTexture(RenderCommand* cmd) {
  auto *c = static_cast<CmdActivateTexture*>(cmd);
  auto it = texture_map_.find(c->id);
  if (it != texture_map_.end())
    glBindTexture(GL_TEXTURE_2D, it->second);
  // TODO: error handling
}

void Renderer::HandleCmdCreateGeometry(RenderCommand* cmd) {
  auto *c = static_cast<CmdCreateGeometry*>(cmd);
  auto it = geometry_map_.find(c->id);
  if (it != geometry_map_.end())
    return; // TODO: error handling

  // Verify that we have a valid layout and get the total byte size per vertex.
  GLuint vertexSize = GetVertexSize(c->vertex_description);
  if (!vertexSize) {
    LOG << "Invalid vertex layout";
    return;  // TODO: error handling
  }

  GLuint vertexArrayId = 0;
  if (SupportsVAO()) {
    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);
  }

  // Create the vertex buffer and upload the data.
  GLuint vertexBufferId = 0;
  glGenBuffers(1, &vertexBufferId);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
  glBufferData(GL_ARRAY_BUFFER, c->num_vertices * vertexSize, c->vertices,
               GL_STATIC_DRAW);

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  std::vector<Geometry::Element> vertexLayout;
  if (!SetupVertexLayout(c->vertex_description, vertexSize, SupportsVAO(), vertexLayout))
    return; // TODO: Error handling

  // Create the index buffer and upload the data.
  GLuint indexBufferId = 0;
  GLenum indexType = GL_NONE;
  if (c->indices) {
    // it->second.indexType = c->index_description;
    indexType = c->index_description;
    glGenBuffers(1, &indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, c->num_indices * GetIndexSize(indexType),
                 c->indices, GL_STATIC_DRAW);
  }

  if (vertexArrayId) {
    // De-activate the buffer again and we're done.
    glBindVertexArray(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  geometry_map_[c->id] = {
    c->num_vertices,
    c->num_indices,
    c->primitive,
    indexType,
    vertexLayout,
    vertexSize,
    vertexArrayId,
    vertexBufferId,
    indexBufferId
  };
}

void Renderer::HandleCmdDestroyGeometry(RenderCommand* cmd) {
  auto *c = static_cast<CmdDestroyGeometry*>(cmd);
  auto it = geometry_map_.find(c->id);
  if (it == geometry_map_.end())
    return; // TODO: error handling

  if (it->second.indexBufferId)
    glDeleteBuffers(1, &(it->second.indexBufferId));
  if (it->second.vertexBufferId)
    glDeleteBuffers(1, &(it->second.vertexBufferId));
  if (it->second.vertexArrayId)
    glDeleteVertexArrays(1, &(it->second.vertexArrayId));
  geometry_map_.erase(it);}

void Renderer::HandleCmdDrawGeometry(RenderCommand* cmd) {
  auto *c = static_cast<CmdDrawGeometry*>(cmd);
  auto it = geometry_map_.find(c->id);
  if (it == geometry_map_.end())
    return; // TODO: error handling

  // Set up the vertex data.
  if (it->second.vertexArrayId)
    glBindVertexArray(it->second.vertexArrayId);
  else {
    glBindBuffer(GL_ARRAY_BUFFER, it->second.vertexBufferId);
    for (GLuint attributeIndex = 0; attributeIndex < (GLuint)it->second.vertexLayout.size();
         ++attributeIndex) {
      Geometry::Element &e = it->second.vertexLayout[attributeIndex];
      glEnableVertexAttribArray(attributeIndex);
      glVertexAttribPointer(attributeIndex, e.numElements, e.type, GL_FALSE,
                            it->second.vertexSize, (const GLvoid *)e.vertexOffset);
    }

    if (it->second.numIndices > 0)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.indexBufferId);
  }

  // Draw the primitive.
  if (it->second.numIndices > 0)
    glDrawElements(it->second.primitive, it->second.numIndices, it->second.indexType, NULL);
  else
    glDrawArrays(it->second.primitive, 0, it->second.numVertices);

  // Clean up states.
  if (it->second.vertexArrayId)
    glBindVertexArray(0);
  else {
    for (GLuint attributeIndex = 0; attributeIndex < (GLuint)it->second.vertexLayout.size();
         ++attributeIndex)
      glDisableVertexAttribArray(attributeIndex);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

void Renderer::HandleCmdCreateShader(RenderCommand* cmd) {
  auto *c = static_cast<CmdCreateShader*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end())
    return; // TODO: Error handling.

  GLuint vertexShader = CreateShader(c->vertex_source.get(), GL_VERTEX_SHADER);
  if (!vertexShader)
    return; // TODO: Error handling.

  GLuint fragmentShader = CreateShader(c->fragment_source.get(), GL_FRAGMENT_SHADER);
  if (!fragmentShader)
    return; // TODO: Error handling.

  GLuint id = glCreateProgram();
  if (id) {
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    if (!BindAttributeLocation(id, c->vertex_description))
      return; // TODO: Error handling.

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
          LOG << "Could not link program:\n" <<  buffer;
          free(buffer);
        }
      }
      glDeleteProgram(id);
      return; // TODO: Error handling.
    }
  }

  shader_map_[c->id] = { id, {} };
}

void Renderer::HandleCmdDestroyShader(RenderCommand* cmd) {
  auto *c = static_cast<CmdDestroyShader*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end()) {
    glDeleteProgram(it->second.id);
    shader_map_.erase(it);
  }
}

void Renderer::HandleCmdActivateShader(RenderCommand* cmd) {
  auto *c = static_cast<CmdActivateShader*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end())
    glUseProgram(it->second.id);
}

void Renderer::HandleCmdSetUniformVec2(RenderCommand* cmd) {
  auto *c = static_cast<CmdSetUniformVec2*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end()) {
    GLint index = GetUniformLocation(it->second.id, c->name, it->second.uniforms);
    if (index >= 0)
      glUniform2fv(index, 1, c->v.GetData());
  }
}

void Renderer::HandleCmdSetUniformVec3(RenderCommand* cmd) {
  auto *c = static_cast<CmdSetUniformVec3*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end()) {
    GLint index = GetUniformLocation(it->second.id, c->name, it->second.uniforms);
    if (index >= 0)
      glUniform3fv(index, 1, c->v.GetData());
  }
}

void Renderer::HandleCmdSetUniformInt(RenderCommand* cmd) {
  auto *c = static_cast<CmdSetUniformInt*>(cmd);
  auto it = shader_map_.find(c->id);
  if (it != shader_map_.end()) {
    GLint index = GetUniformLocation(it->second.id, c->name, it->second.uniforms);
    if (index >= 0)
      glUniform1i(index, c->i);
  }
}

bool Renderer::SetupVertexLayout(const std::string &vertexDescription,
                                 GLuint vertexSize, bool useVAO,
                                 std::vector<Geometry::Element> &vertexLayout) {
  GLuint attributeIndex = 0;
  size_t vertexOffset = 0;

  // Parse the layout.
  char buffer[32];
  strcpy(buffer, vertexDescription.c_str());
  char *token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    // There's a limitation of 16 attributes in OpenGL ES 2.0
    if (attributeIndex >= 16)
      return false;

    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    GLsizei numElements = token[1] - '1' + 1;
    if (numElements < 1 || numElements > 4)
      return false;

    // The data type is needed, the most common ones are supported.
    GLenum type;
    size_t typeSize;
    switch (token[2]) {
    case 'b': type = GL_UNSIGNED_BYTE;  typeSize = sizeof(GLbyte);    break;
    case 'f': type = GL_FLOAT;          typeSize = sizeof(GLfloat);   break;
    case 'i': type = GL_INT;            typeSize = sizeof(GLint);     break;
    case 's': type = GL_SHORT;          typeSize = sizeof(GLshort);   break;
    case 'u': type = GL_UNSIGNED_INT;   typeSize = sizeof(GLuint);    break;
    case 'w': type = GL_UNSIGNED_SHORT; typeSize = sizeof(GLushort);  break;
    default:  return false;
    }

    // We got all we need to define this attribute.
    if (useVAO) {
      // This will be saved into the vertex array object.
      glEnableVertexAttribArray(attributeIndex);
      glVertexAttribPointer(attributeIndex, numElements, type, GL_FALSE,
                            vertexSize, (const GLvoid *)vertexOffset);
    } else {
      // Need to keep this information for when rendering.
      Geometry::Element element;
      element.numElements   = numElements;
      element.type          = type;
      element.vertexOffset  = vertexOffset;
      vertexLayout.push_back(element);
    }

    // Move on to the next attribute.
    ++attributeIndex;
    vertexOffset += numElements * typeSize;
    token = strtok(NULL, kLayoutDelimiter);
  }

  return true;
}

GLuint Renderer::CreateShader(const char *source, GLenum type) {
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
          LOG << "Could not compile shader " << type << ":\n" << buffer;
          free(buffer);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }

  return shader;
}

bool Renderer::BindAttributeLocation(GLuint id, const std::string &vertexDescription) {
  int current = 0,
      texCoord = 0;

  // Parse the description.
  char buffer[32];
  strcpy(buffer, vertexDescription.c_str());
  char *token = strtok(buffer, kLayoutDelimiter);

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
      LOG << "Unknown attribute: " << token;
      return false;
    }

    token = strtok(NULL, kLayoutDelimiter);
  }

  // We need at least one position attribute.
  return current > 0;
}

GLint Renderer::GetUniformLocation(GLuint id, const std::string &name, std::unordered_map<std::string, GLuint> &uniforms) {
  // Check if we've encountered this uniform before.
  auto i = uniforms.find(name);
  GLint index;
  if (i != uniforms.end()) {
    // Yes, we already have the mapping.
    index = i->second;
  } else {
    // No, ask the driver for the mapping and save it.
    index = glGetUniformLocation(id, name.c_str());
    if (index >= 0)
      uniforms[name] = index;
    else
      LOG << "Cannot find uniform " << name.c_str() << " (shader: " << id << ")";
  }
  return index;
}

std::unordered_set<std::string> Renderer::SetupExtensions() {
  std::stringstream stream((const char*)glGetString(GL_EXTENSIONS));
  std::string token;
  std::unordered_set<std::string> extensions;
  while (std::getline(stream, token, ' '))
    extensions.insert(token);

#if 0
  LOG << "  extensions:";
  for (auto& ext : extensions)
    LOG << "    " << ext.c_str());
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

  return extensions;
}

void Renderer::EnableBlend() {
  auto cmd = std::make_unique<CmdEableBlend>();
  EnqueueCommand(std::move(cmd));
}

void Renderer::Clear(const std::array<float, 4>& rgba) {
  auto cmd = std::make_unique<CmdClear>();
  cmd->rgba = rgba;
  EnqueueCommand(std::move(cmd));
}

void Renderer::Present() {
  auto cmd = std::make_unique<CmdPresent>();
  EnqueueCommand(std::move(cmd));
}

void Renderer::ContextLost() {}

void Renderer::LogVersion() {
  LOG << "OpenGL:";
  LOG << "  vendor:         " << (const char*)glGetString(GL_VENDOR);
  LOG << "  renderer:       " << (const char*)glGetString(GL_RENDERER);
  LOG << "  version:        " << (const char*)glGetString(GL_VERSION);
  LOG << "  shader version: " <<
      (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
}

}  // namespace engine
