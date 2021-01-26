#include "renderer_opengl.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <unordered_set>

#include "../../../base/log.h"
#include "../../../base/vecmath.h"
#ifdef THREADED_RENDERING
#include "../../../base/task_runner.h"
#endif  // THREADED_RENDERING
#include "../../image.h"
#include "../../mesh.h"
#include "../../shader_source.h"
#include "../geometry.h"
#include "../shader.h"
#include "../texture.h"
#include "render_command.h"

using namespace base;

namespace {

constexpr GLenum kGlPrimitive[eng::kPrimitive_Max] = {GL_TRIANGLES,
                                                      GL_TRIANGLE_STRIP};

constexpr GLenum kGlDataType[eng::kDataType_Max] = {
    GL_UNSIGNED_BYTE, GL_FLOAT,        GL_INT,
    GL_SHORT,         GL_UNSIGNED_INT, GL_UNSIGNED_SHORT};

const std::string kAttributeNames[eng::kAttribType_Max] = {
    "in_color", "in_normal", "in_position", "in_tex_coord"};

}  // namespace

namespace eng {

#ifdef THREADED_RENDERING
RendererOpenGL::RendererOpenGL()
    : main_thread_task_runner_(TaskRunner::GetThreadLocalTaskRunner()) {}
#else
RendererOpenGL::RendererOpenGL() = default;
#endif  // THREADED_RENDERING

RendererOpenGL::~RendererOpenGL() = default;

void RendererOpenGL::CreateGeometry(std::shared_ptr<void> impl_data,
                                    std::unique_ptr<Mesh> mesh) {
  auto cmd = std::make_unique<CmdCreateGeometry>();
  cmd->mesh = std::move(mesh);
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::DestroyGeometry(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdDestroyGeometry>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::Draw(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdDrawGeometry>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::UpdateTexture(std::shared_ptr<void> impl_data,
                                   std::unique_ptr<Image> image) {
  auto cmd = std::make_unique<CmdUpdateTexture>();
  cmd->image = std::move(image);
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::DestroyTexture(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdDestoryTexture>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::ActivateTexture(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdActivateTexture>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::CreateShader(std::shared_ptr<void> impl_data,
                                  std::unique_ptr<ShaderSource> source,
                                  const VertexDescripton& vertex_description,
                                  Primitive primitive,
                                  bool enable_depth_test) {
  auto cmd = std::make_unique<CmdCreateShader>();
  cmd->source = std::move(source);
  cmd->vertex_description = vertex_description;
  cmd->impl_data = impl_data;
  cmd->enable_depth_test = enable_depth_test;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::DestroyShader(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdDestroyShader>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::ActivateShader(std::shared_ptr<void> impl_data) {
  auto cmd = std::make_unique<CmdActivateShader>();
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector2f& val) {
  auto cmd = std::make_unique<CmdSetUniformVec2>();
  cmd->name = name;
  cmd->v = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector3f& val) {
  auto cmd = std::make_unique<CmdSetUniformVec3>();
  cmd->name = name;
  cmd->v = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector4f& val) {
  auto cmd = std::make_unique<CmdSetUniformVec4>();
  cmd->name = name;
  cmd->v = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Matrix4f& val) {
  auto cmd = std::make_unique<CmdSetUniformMat4>();
  cmd->name = name;
  cmd->m = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                float val) {
  auto cmd = std::make_unique<CmdSetUniformFloat>();
  cmd->name = name;
  cmd->f = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                int val) {
  auto cmd = std::make_unique<CmdSetUniformInt>();
  cmd->name = name;
  cmd->i = val;
  cmd->impl_data = impl_data;
  EnqueueCommand(std::move(cmd));
}

void RendererOpenGL::Present() {
  EnqueueCommand(std::make_unique<CmdPresent>());
#ifdef THREADED_RENDERING
  draw_complete_semaphore_.Acquire();
#endif  // THREADED_RENDERING
  fps_++;
}

void RendererOpenGL::ContextLost() {
  LOG << "Context lost.";

#ifdef THREADED_RENDERING
  global_commands_.clear();
  draw_commands_[0].clear();
  draw_commands_[1].clear();

  main_thread_task_runner_->PostTask(
      HERE, std::bind(&RendererOpenGL::InvalidateAllResources, this));
  main_thread_task_runner_->PostTask(HERE, context_lost_cb_);
#else
  InvalidateAllResources();
  context_lost_cb_();
#endif  // THREADED_RENDERING
}

std::unique_ptr<RenderResource> RendererOpenGL::CreateResource(
    RenderResourceFactoryBase& factory) {
  static unsigned last_id = 0;

  // Set implementation specific data. This data will be sent with render
  // commands to the render thread and sould not be used in any other thread.
  std::shared_ptr<void> impl_data;
  if (factory.IsTypeOf<Geometry>())
    impl_data = std::make_shared<GeometryOpenGL>();
  else if (factory.IsTypeOf<Shader>())
    impl_data = std::make_shared<ShaderOpenGL>();
  else if (factory.IsTypeOf<Texture>())
    impl_data = std::make_shared<TextureOpenGL>();
  else
    NOTREACHED << "- Unknown resource type.";

  unsigned resource_id = ++last_id;
  auto resource = factory.Create(resource_id, impl_data, this);
  resources_[resource_id] = resource.get();
  return resource;
}

void RendererOpenGL::ReleaseResource(unsigned resource_id) {
  auto it = resources_.find(resource_id);
  if (it != resources_.end())
    resources_.erase(it);
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

  LOG << "OpenGL:";
  LOG << "  vendor:         " << (const char*)glGetString(GL_VENDOR);
  LOG << "  renderer:       " << renderer;
  LOG << "  version:        " << version;
  LOG << "  shader version: "
      << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  LOG << "Screen size: " << screen_width_ << ", " << screen_height_;

  // Setup extensions.
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

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
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
    LOG << "NPOT not supported.";

  if (vertex_array_objects_)
    LOG << "Supports Vertex Array Objects.";

  LOG << "TextureCompression:";
  LOG << "  atc:   " << texture_compression_.atc;
  LOG << "  dxt1:  " << texture_compression_.dxt1;
  LOG << "  etc1:  " << texture_compression_.etc1;
  LOG << "  s3tc:  " << texture_compression_.s3tc;

  glViewport(0, 0, screen_width_, screen_height_);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0, 0, 0, 1);

  return true;
}

void RendererOpenGL::InvalidateAllResources() {
  for (auto& r : resources_)
    r.second->Destroy();
}

bool RendererOpenGL::StartRenderThread() {
#ifdef THREADED_RENDERING
  LOG << "Starting render thread.";

  global_commands_.clear();
  draw_commands_[0].clear();
  draw_commands_[1].clear();
  terminate_render_thread_ = false;

  std::promise<bool> promise;
  std::future<bool> future = promise.get_future();
  render_thread_ =
      std::thread(&RendererOpenGL::RenderThreadMain, this, std::move(promise));
  future.wait();
  return future.get();
#else
  LOG << "Single threaded rendering.";
  return InitInternal();
#endif  // THREADED_RENDERING
}

void RendererOpenGL::TerminateRenderThread() {
#ifdef THREADED_RENDERING
  if (terminate_render_thread_)
    return;

  // Notify worker thread and wait for it to terminate.
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    terminate_render_thread_ = true;
  }
  cv_.notify_one();
  LOG << "Terminating render thread";
  render_thread_.join();
#else
  ShutdownInternal();
#endif  // THREADED_RENDERING
}

#ifdef THREADED_RENDERING

void RendererOpenGL::RenderThreadMain(std::promise<bool> promise) {
  promise.set_value(InitInternal());

  std::deque<std::unique_ptr<RenderCommand>> cq[2];
  for (;;) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      cv_.wait(scoped_lock, [&]() -> bool {
        return !global_commands_.empty() || !draw_commands_[0].empty() ||
               terminate_render_thread_;
      });
      if (terminate_render_thread_) {
        ShutdownInternal();
        return;
      }
      cq[0].swap(global_commands_);
      cq[1].swap(draw_commands_[0]);
    }

#if 0
    LOG << "qlobal queue size: " << (int)cq[0].size();
    LOG << "draw queue size: " << (int)cq[1].size();
#endif

    for (int i = 0; i < 2; ++i) {
      while (!cq[i].empty()) {
        ProcessCommand(cq[i].front().get());
        cq[i].pop_front();
      }
    }
  }
}

#endif  // THREADED_RENDERING

void RendererOpenGL::EnqueueCommand(std::unique_ptr<RenderCommand> cmd) {
#ifdef THREADED_RENDERING
  if (cmd->global) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      global_commands_.push_back(std::move(cmd));
    }
    cv_.notify_one();
    return;
  }

  bool new_frame = cmd->cmd_id == CmdPresent::CMD_ID;
  draw_commands_[1].push_back(std::move(cmd));
  if (new_frame) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      draw_commands_[0].swap(draw_commands_[1]);
    }
    cv_.notify_one();
    draw_commands_[1].clear();
  }
#else
  ProcessCommand(cmd.get());
#endif  // THREADED_RENDERING
}

void RendererOpenGL::ProcessCommand(RenderCommand* cmd) {
#if 0
  LOG << "Processing command: " << cmd->cmd_name.c_str();
#endif

  switch (cmd->cmd_id) {
    case CmdPresent::CMD_ID:
      HandleCmdPresent(cmd);
      break;
    case CmdUpdateTexture::CMD_ID:
      HandleCmdUpdateTexture(cmd);
      break;
    case CmdDestoryTexture::CMD_ID:
      HandleCmdDestoryTexture(cmd);
      break;
    case CmdActivateTexture::CMD_ID:
      HandleCmdActivateTexture(cmd);
      break;
    case CmdCreateGeometry::CMD_ID:
      HandleCmdCreateGeometry(cmd);
      break;
    case CmdDestroyGeometry::CMD_ID:
      HandleCmdDestroyGeometry(cmd);
      break;
    case CmdDrawGeometry::CMD_ID:
      HandleCmdDrawGeometry(cmd);
      break;
    case CmdCreateShader::CMD_ID:
      HandleCmdCreateShader(cmd);
      break;
    case CmdDestroyShader::CMD_ID:
      HandleCmdDestroyShader(cmd);
      break;
    case CmdActivateShader::CMD_ID:
      HandleCmdActivateShader(cmd);
      break;
    case CmdSetUniformVec2::CMD_ID:
      HandleCmdSetUniformVec2(cmd);
      break;
    case CmdSetUniformVec3::CMD_ID:
      HandleCmdSetUniformVec3(cmd);
      break;
    case CmdSetUniformVec4::CMD_ID:
      HandleCmdSetUniformVec4(cmd);
      break;
    case CmdSetUniformMat4::CMD_ID:
      HandleCmdSetUniformMat4(cmd);
      break;
    case CmdSetUniformFloat::CMD_ID:
      HandleCmdSetUniformFloat(cmd);
      break;
    case CmdSetUniformInt::CMD_ID:
      HandleCmdSetUniformInt(cmd);
      break;
    default:
      NOTREACHED << "- Unknown render command: " << cmd->cmd_id;
  }
}

void RendererOpenGL::HandleCmdUpdateTexture(RenderCommand* cmd) {
  auto* c = static_cast<CmdUpdateTexture*>(cmd);
  auto impl_data = reinterpret_cast<TextureOpenGL*>(c->impl_data.get());
  bool new_texture = impl_data->id == 0;

  GLuint gl_id = 0;
  if (new_texture)
    glGenTextures(1, &gl_id);
  else
    gl_id = impl_data->id;

  glBindTexture(GL_TEXTURE_2D, gl_id);
  if (c->image->IsCompressed()) {
    GLenum format = 0;
    switch (c->image->GetFormat()) {
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
        NOTREACHED << "- Unhandled texure format: " << c->image->GetFormat();
    }

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, c->image->GetWidth(),
                           c->image->GetHeight(), 0, c->image->GetSize(),
                           c->image->GetBuffer());

    // Sometimes the first glCompressedTexImage2D call after context-lost
    // generates GL_INVALID_VALUE.
    GLenum err = glGetError();
    if (err == GL_INVALID_VALUE) {
      glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, c->image->GetWidth(),
                             c->image->GetHeight(), 0, c->image->GetSize(),
                             c->image->GetBuffer());
      err = glGetError();
    }

    if (err != GL_NO_ERROR)
      LOG << "GL ERROR after glCompressedTexImage2D: " << (int)err;
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c->image->GetWidth(),
                 c->image->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 c->image->GetBuffer());
  }

  if (new_texture) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    *impl_data = {gl_id};
  }
}

void RendererOpenGL::HandleCmdDestoryTexture(RenderCommand* cmd) {
  auto* c = static_cast<CmdDestoryTexture*>(cmd);
  auto impl_data = reinterpret_cast<TextureOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    glDeleteTextures(1, &(impl_data->id));
    *impl_data = {};
  }
}

void RendererOpenGL::HandleCmdActivateTexture(RenderCommand* cmd) {
  auto* c = static_cast<CmdActivateTexture*>(cmd);
  auto impl_data = reinterpret_cast<TextureOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0 && impl_data->id != active_texture_id_) {
    glBindTexture(GL_TEXTURE_2D, impl_data->id);
    active_texture_id_ = impl_data->id;
  }
}

void RendererOpenGL::HandleCmdCreateGeometry(RenderCommand* cmd) {
  auto* c = static_cast<CmdCreateGeometry*>(cmd);
  auto impl_data = reinterpret_cast<GeometryOpenGL*>(c->impl_data.get());
  if (impl_data->vertex_buffer_id > 0)
    return;

  // Verify that we have a valid layout and get the total byte size per vertex.
  GLuint vertex_size = c->mesh->GetVertexSize();
  if (!vertex_size) {
    LOG << "Invalid vertex layout";
    return;
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
  glBufferData(GL_ARRAY_BUFFER, c->mesh->num_vertices() * vertex_size,
               c->mesh->GetVertices(), GL_STATIC_DRAW);

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  std::vector<GeometryOpenGL::Element> vertex_layout;
  if (!SetupVertexLayout(c->mesh->vertex_description(), vertex_size,
                         vertex_array_objects_, vertex_layout))
    return;

  // Create the index buffer and upload the data.
  GLuint index_buffer_id = 0;
  if (c->mesh->GetIndices()) {
    glGenBuffers(1, &index_buffer_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 c->mesh->num_indices() * c->mesh->GetIndexSize(),
                 c->mesh->GetIndices(), GL_STATIC_DRAW);
  }

  if (vertex_array_id) {
    // De-activate the buffer again and we're done.
    glBindVertexArray(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  *impl_data = {(GLsizei)c->mesh->num_vertices(),
                (GLsizei)c->mesh->num_indices(),
                kGlPrimitive[c->mesh->primitive()],
                kGlDataType[c->mesh->index_description()],
                vertex_layout,
                vertex_size,
                vertex_array_id,
                vertex_buffer_id,
                index_buffer_id};
}

void RendererOpenGL::HandleCmdDestroyGeometry(RenderCommand* cmd) {
  auto* c = static_cast<CmdDestroyGeometry*>(cmd);
  auto impl_data = reinterpret_cast<GeometryOpenGL*>(c->impl_data.get());
  if (impl_data->vertex_buffer_id == 0)
    return;

  if (impl_data->index_buffer_id)
    glDeleteBuffers(1, &(impl_data->index_buffer_id));
  if (impl_data->vertex_buffer_id)
    glDeleteBuffers(1, &(impl_data->vertex_buffer_id));
  if (impl_data->vertex_array_id)
    glDeleteVertexArrays(1, &(impl_data->vertex_array_id));

  *impl_data = {};
}

void RendererOpenGL::HandleCmdDrawGeometry(RenderCommand* cmd) {
  auto* c = static_cast<CmdDrawGeometry*>(cmd);
  auto impl_data = reinterpret_cast<GeometryOpenGL*>(c->impl_data.get());
  if (impl_data->vertex_buffer_id == 0)
    return;

  // Set up the vertex data.
  if (impl_data->vertex_array_id)
    glBindVertexArray(impl_data->vertex_array_id);
  else {
    glBindBuffer(GL_ARRAY_BUFFER, impl_data->vertex_buffer_id);
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)impl_data->vertex_layout.size();
         ++attribute_index) {
      GeometryOpenGL::Element& e = impl_data->vertex_layout[attribute_index];
      glEnableVertexAttribArray(attribute_index);
      glVertexAttribPointer(attribute_index, e.num_elements, e.type, GL_FALSE,
                            impl_data->vertex_size,
                            (const GLvoid*)e.vertex_offset);
    }

    if (impl_data->num_indices > 0)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl_data->index_buffer_id);
  }

  // Draw the primitive.
  if (impl_data->num_indices > 0)
    glDrawElements(impl_data->primitive, impl_data->num_indices,
                   impl_data->index_type, NULL);
  else
    glDrawArrays(impl_data->primitive, 0, impl_data->num_vertices);

  // Clean up states.
  if (impl_data->vertex_array_id)
    glBindVertexArray(0);
  else {
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)impl_data->vertex_layout.size();
         ++attribute_index)
      glDisableVertexAttribArray(attribute_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

void RendererOpenGL::HandleCmdCreateShader(RenderCommand* cmd) {
  auto* c = static_cast<CmdCreateShader*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0)
    return;

  GLuint vertex_shader =
      CreateShader(c->source->GetVertexSource(), GL_VERTEX_SHADER);
  if (!vertex_shader)
    return;

  GLuint fragment_shader =
      CreateShader(c->source->GetFragmentSource(), GL_FRAGMENT_SHADER);
  if (!fragment_shader)
    return;

  GLuint id = glCreateProgram();
  if (id) {
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    if (!BindAttributeLocation(id, c->vertex_description))
      return;

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
          LOG << "Could not link program:\n" << buffer;
          free(buffer);
        }
      }
      glDeleteProgram(id);
      return;
    }
  }

  *impl_data = {id, {}, c->enable_depth_test};
}

void RendererOpenGL::HandleCmdDestroyShader(RenderCommand* cmd) {
  auto* c = static_cast<CmdDestroyShader*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    glDeleteProgram(impl_data->id);
    *impl_data = {};
  }
}

void RendererOpenGL::HandleCmdActivateShader(RenderCommand* cmd) {
  auto* c = static_cast<CmdActivateShader*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0 && impl_data->id != active_shader_id_) {
    glUseProgram(impl_data->id);
    active_shader_id_ = impl_data->id;
    if (impl_data->enable_depth_test)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
  }
}

void RendererOpenGL::HandleCmdSetUniformVec2(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformVec2*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniform2fv(index, 1, c->v.GetData());
  }
}

void RendererOpenGL::HandleCmdSetUniformVec3(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformVec3*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniform3fv(index, 1, c->v.GetData());
  }
}

void RendererOpenGL::HandleCmdSetUniformVec4(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformVec4*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniform4fv(index, 1, c->v.GetData());
  }
}

void RendererOpenGL::HandleCmdSetUniformMat4(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformMat4*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniformMatrix4fv(index, 1, GL_FALSE, c->m.GetData());
  }
}

void RendererOpenGL::HandleCmdSetUniformFloat(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformFloat*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniform1f(index, c->f);
  }
}

void RendererOpenGL::HandleCmdSetUniformInt(RenderCommand* cmd) {
  auto* c = static_cast<CmdSetUniformInt*>(cmd);
  auto impl_data = reinterpret_cast<ShaderOpenGL*>(c->impl_data.get());
  if (impl_data->id > 0) {
    GLint index =
        GetUniformLocation(impl_data->id, c->name, impl_data->uniforms);
    if (index >= 0)
      glUniform1i(index, c->i);
  }
}

bool RendererOpenGL::SetupVertexLayout(
    const VertexDescripton& vd,
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
      glVertexAttribPointer(attribute_index, num_elements, type, GL_FALSE,
                            vertex_size, (const GLvoid*)vertex_offset);
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

bool RendererOpenGL::BindAttributeLocation(GLuint id,
                                           const VertexDescripton& vd) {
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
    std::unordered_map<std::string, GLuint>& uniforms) {
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
      LOG << "Cannot find uniform " << name.c_str() << " (shader: " << id
          << ")";
  }
  return index;
}

}  // namespace eng
