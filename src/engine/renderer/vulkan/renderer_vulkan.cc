#include "renderer_vulkan.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "../../../base/log.h"
#include "../../../base/vecmath.h"
#include "../../../third_party/glslang/SPIRV/GlslangToSpv.h"
#include "../../../third_party/glslang/StandAlone/ResourceLimits.h"
#include "../../../third_party/glslang/glslang/Include/Types.h"
#include "../../../third_party/glslang/glslang/Public/ShaderLang.h"
#include "../../../third_party/spirv-reflect/spirv_reflect.h"
#include "../../image.h"
#include "../../mesh.h"
#include "../../shader_source.h"
#include "../geometry.h"
#include "../shader.h"
#include "../texture.h"

using namespace base;

namespace {

using VertexInputDescription =
    std::tuple<std::vector<VkVertexInputBindingDescription>,
               std::vector<VkVertexInputAttributeDescription>>;

constexpr VkPrimitiveTopology kVkPrimitiveType[eng::kPrimitive_Max] = {
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
};

constexpr VkFormat kVkDataType[eng::kDataType_Max][4] = {
    {
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8B8_UINT,
        VK_FORMAT_R8G8B8A8_UINT,
    },
    {
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
    },
    {
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32A32_SINT,
    },
    {
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16A16_SINT,
    },
    {
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32B32A32_UINT,
    },
    {
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R16G16B16A16_UINT,
    },
};

constexpr size_t kMaxDescriptorsPerPool = 64;

std::vector<uint8_t> CompileGlsl(EShLanguage stage,
                                 const char* source_code,
                                 std::string* error) {
  const int kClientInputSemanticsVersion = 100;  // maps to #define VULKAN 100
  const int kDefaultVersion = 450;

  std::vector<uint8_t> ret;

  glslang::EShTargetClientVersion vulkan_client_version =
      glslang::EShTargetVulkan_1_0;
  glslang::EShTargetLanguageVersion target_version = glslang::EShTargetSpv_1_0;
  glslang::TShader::ForbidIncluder includer;

  glslang::TShader shader(stage);
  const char* cs_strings = source_code;

  shader.setStrings(&cs_strings, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan,
                     kClientInputSemanticsVersion);
  shader.setEnvClient(glslang::EShClientVulkan, vulkan_client_version);
  shader.setEnvTarget(glslang::EShTargetSpv, target_version);

  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
  std::string pre_processed_code;

  // Preprocess
  if (!shader.preprocess(&glslang::DefaultTBuiltInResource, kDefaultVersion,
                         ENoProfile, false, false, messages,
                         &pre_processed_code, includer)) {
    if (error) {
      (*error) = "Failed pre-process:\n";
      (*error) += shader.getInfoLog();
      (*error) += "\n";
      (*error) += shader.getInfoDebugLog();
    }

    return ret;
  }
  cs_strings = pre_processed_code.c_str();
  shader.setStrings(&cs_strings, 1);

  // Parse
  if (!shader.parse(&glslang::DefaultTBuiltInResource, kDefaultVersion, false,
                    messages)) {
    if (error) {
      (*error) = "Failed parse:\n";
      (*error) += shader.getInfoLog();
      (*error) += "\n";
      (*error) += shader.getInfoDebugLog();
    }
    return ret;
  }

  // link
  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages)) {
    if (error) {
      (*error) = "Failed link:\n";
      (*error) += program.getInfoLog();
      (*error) += "\n";
      (*error) += program.getInfoDebugLog();
    }

    return ret;
  }

  std::vector<uint32_t> spirv;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spv_options;
  glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger,
                        &spv_options);

  ret.resize(spirv.size() * sizeof(uint32_t));
  {
    uint8_t* w = ret.data();
    memcpy(w, &spirv[0], spirv.size() * sizeof(uint32_t));
  }

  return ret;
}

VertexInputDescription GetVertexInputDescription(
    const eng::VertexDescripton& vd) {
  unsigned vertex_offset = 0;
  unsigned location = 0;

  std::vector<VkVertexInputAttributeDescription> attributes;

  for (auto& attr : vd) {
    auto [attrib_type, data_type, num_elements, type_size] = attr;

    VkVertexInputAttributeDescription attribute;
    attribute.location = location++;
    attribute.binding = 0;
    attribute.format = kVkDataType[data_type][num_elements - 1];
    attribute.offset = vertex_offset;
    attributes.push_back(attribute);

    vertex_offset += num_elements * type_size;
  }

  std::vector<VkVertexInputBindingDescription> bindings(1);
  bindings[0].binding = 0;
  bindings[0].stride = vertex_offset;
  bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return std::make_tuple(std::move(bindings), std::move(attributes));
}

VkIndexType GetIndexType(eng::DataType data_type) {
  switch (data_type) {
    case eng::kDataType_UInt:
      return VK_INDEX_TYPE_UINT32;
    case eng::kDataType_UShort:
      return VK_INDEX_TYPE_UINT16;
    default:
      break;
  }
  NOTREACHED << "Invalid index type: " << data_type;
  return VK_INDEX_TYPE_UINT16;
}

VkFormat GetImageFormat(eng::Image::Format format) {
  switch (format) {
    case eng::Image::kRGBA32:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case eng::Image::kDXT1:
      return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case eng::Image::kDXT5:
      return VK_FORMAT_BC3_UNORM_BLOCK;
    case eng::Image::kETC1:
    case eng::Image::kATC:
    case eng::Image::kATCIA:
    default:
      break;
  }
  NOTREACHED << "Invalid format: " << format;
  return VK_FORMAT_R8G8B8A8_UNORM;
}

void GetBlockSizeForImageFormat(VkFormat format, int& size, int& height) {
  switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
      size = 4;
      height = 1;
      return;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
      size = 8;
      height = 4;
      return;
    case VK_FORMAT_BC3_UNORM_BLOCK:
      size = 16;
      height = 4;
      return;
    default:
      break;
  }
  NOTREACHED << "Invalid format: " << format;
}

void GetNumBlocksForImageFormat(VkFormat format,
                                int& num_blocks_x,
                                int& num_blocks_y) {
  switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
      return;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
      num_blocks_x = (num_blocks_x + 3) / 4;
      num_blocks_y = (num_blocks_y + 3) / 4;
      return;
    default:
      break;
  }
  NOTREACHED << "Invalid format: " << format;
}

}  // namespace

namespace eng {

RendererVulkan::RendererVulkan() = default;

RendererVulkan::~RendererVulkan() = default;

void RendererVulkan::CreateGeometry(std::shared_ptr<void> impl_data,
                                    std::unique_ptr<Mesh> mesh) {
  auto geometry = reinterpret_cast<GeometryVulkan*>(impl_data.get());
  geometry->num_vertices = mesh->num_vertices();
  size_t vertex_data_size = mesh->GetVertexSize() * geometry->num_vertices;
  size_t index_data_size = 0;

  if (mesh->GetIndices()) {
    geometry->num_indices = mesh->num_indices();
    geometry->index_type = GetIndexType(mesh->index_description());
    index_data_size = mesh->GetIndexSize() * geometry->num_indices;
  }
  size_t data_size = vertex_data_size + index_data_size;

  AllocateBuffer(geometry->buffer, data_size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY);

  task_runner_.PostTask(HERE, std::bind(&RendererVulkan::UpdateBuffer, this,
                                        std::get<0>(geometry->buffer), 0,
                                        mesh->GetVertices(), vertex_data_size));
  if (geometry->num_indices > 0) {
    geometry->indices_offset = vertex_data_size;
    task_runner_.PostTask(
        HERE, std::bind(&RendererVulkan::UpdateBuffer, this,
                        std::get<0>(geometry->buffer), geometry->indices_offset,
                        mesh->GetIndices(), index_data_size));
  }
  task_runner_.PostTask(HERE,
                        std::bind(&RendererVulkan::BufferMemoryBarrier, this,
                                  std::get<0>(geometry->buffer), 0, data_size,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                  VK_ACCESS_TRANSFER_WRITE_BIT,
                                  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));
  task_runner_.PostTask(HERE, [&, mesh = mesh.release()]() {
    // Transfer mesh ownership to the background thread.
    std::unique_ptr<Mesh> own(mesh);
  });
  semaphore_.Release();
}

void RendererVulkan::DestroyGeometry(std::shared_ptr<void> impl_data) {
  auto geometry = reinterpret_cast<GeometryVulkan*>(impl_data.get());
  FreeBuffer(std::move(geometry->buffer));
  geometry = {};
}

void RendererVulkan::Draw(std::shared_ptr<void> impl_data) {
  auto geometry = reinterpret_cast<GeometryVulkan*>(impl_data.get());
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(frames_[current_frame_].draw_command_buffer, 0, 1,
                         &std::get<0>(geometry->buffer), &offset);
  if (geometry->num_indices > 0) {
    vkCmdBindIndexBuffer(frames_[current_frame_].draw_command_buffer,
                         std::get<0>(geometry->buffer),
                         geometry->indices_offset, geometry->index_type);
    vkCmdDrawIndexed(frames_[current_frame_].draw_command_buffer,
                     geometry->num_indices, 1, 0, 0, 0);
  } else {
    vkCmdDraw(frames_[current_frame_].draw_command_buffer,
              geometry->num_vertices, 1, 0, 0);
  }
}

void RendererVulkan::UpdateTexture(std::shared_ptr<void> impl_data,
                                   std::unique_ptr<Image> image) {
  auto texture = reinterpret_cast<TextureVulkan*>(impl_data.get());
  VkImageLayout old_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  VkFormat format = GetImageFormat(image->GetFormat());

  if (texture->view != VK_NULL_HANDLE &&
      (texture->width != image->GetWidth() ||
       texture->height != image->GetHeight())) {
    // Size mismatch. Recreate the texture.
    FreeTexture(std::move(texture->image), texture->view,
                std::move(texture->desc_set));
    *texture = {};
  }

  if (texture->view == VK_NULL_HANDLE) {
    CreateTexture(texture->image, texture->view, texture->desc_set, format,
                  image->GetWidth(), image->GetHeight(),
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VMA_MEMORY_USAGE_GPU_ONLY);
    old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture->width = image->GetWidth();
    texture->height = image->GetHeight();
  }

  task_runner_.PostTask(
      HERE,
      std::bind(&RendererVulkan::ImageMemoryBarrier, this,
                std::get<0>(texture->image),
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                old_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
  task_runner_.PostTask(
      HERE, std::bind(&RendererVulkan::UpdateImage, this,
                      std::get<0>(texture->image), format, image->GetBuffer(),
                      image->GetWidth(), image->GetHeight()));
  task_runner_.PostTask(
      HERE, std::bind(&RendererVulkan::ImageMemoryBarrier, this,
                      std::get<0>(texture->image), VK_ACCESS_TRANSFER_WRITE_BIT,
                      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                      0, VK_ACCESS_SHADER_READ_BIT,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
  task_runner_.PostTask(HERE, [&, image = image.release()]() {
    // Transfer image ownership to the background thread.
    std::unique_ptr<Image> own(image);
  });
  semaphore_.Release();
}

void RendererVulkan::DestroyTexture(std::shared_ptr<void> impl_data) {
  auto texture = reinterpret_cast<TextureVulkan*>(impl_data.get());
  FreeTexture(std::move(texture->image), texture->view,
              std::move(texture->desc_set));
  *texture = {};
}

void RendererVulkan::ActivateTexture(std::shared_ptr<void> impl_data) {
  auto texture = reinterpret_cast<TextureVulkan*>(impl_data.get());
  // Keep as pengind and bind later in ActivateShader.
  penging_descriptor_sets_[/*TODO*/ 0] = std::get<0>(texture->desc_set);
}

void RendererVulkan::CreateShader(std::shared_ptr<void> impl_data,
                                  std::unique_ptr<ShaderSource> source,
                                  const VertexDescripton& vertex_description,
                                  Primitive primitive) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());

  auto it = spirv_cache_.find(source->name());
  if (it == spirv_cache_.end()) {
    std::array<std::vector<uint8_t>, 2> spirv;
    std::string error;
    spirv[0] = CompileGlsl(EShLangVertex, source->GetVertexSource(), &error);
    if (!error.empty())
      DLOG << source->name() << " vertex shader compile error: " << error;
    spirv[1] =
        CompileGlsl(EShLangFragment, source->GetFragmentSource(), &error);
    if (!error.empty())
      DLOG << source->name() << " fragment shader compile error: " << error;
    it = spirv_cache_.insert({source->name(), spirv}).first;
  }

  auto& spirv_vertex = it->second[0];
  auto& spirv_fragment = it->second[1];

  VkShaderModule vert_shader_module;
  {
    VkShaderModuleCreateInfo shader_module_info{};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.codeSize = spirv_vertex.size();
    shader_module_info.pCode =
        reinterpret_cast<const uint32_t*>(spirv_vertex.data());

    if (vkCreateShaderModule(device_, &shader_module_info, nullptr,
                             &vert_shader_module) != VK_SUCCESS) {
      DLOG << "vkCreateShaderModule failed!";
      return;
    }
  }

  VkShaderModule frag_shader_module;
  {
    VkShaderModuleCreateInfo shader_module_info{};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.codeSize = spirv_fragment.size();
    shader_module_info.pCode =
        reinterpret_cast<const uint32_t*>(spirv_fragment.data());

    if (vkCreateShaderModule(device_, &shader_module_info, nullptr,
                             &frag_shader_module) != VK_SUCCESS) {
      DLOG << "vkCreateShaderModule failed!";
      return;
    }
  }

  if (!CreatePipelineLayout(shader, spirv_vertex, spirv_fragment))
    return;

  VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
  vert_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
  frag_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vert_shader_stage_info,
                                                    frag_shader_stage_info};

  VertexInputDescription vertex_input =
      GetVertexInputDescription(vertex_description);

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount =
      std::get<0>(vertex_input).size();
  vertex_input_info.vertexAttributeDescriptionCount =
      std::get<1>(vertex_input).size();
  vertex_input_info.pVertexBindingDescriptions =
      std::get<0>(vertex_input).data();
  vertex_input_info.pVertexAttributeDescriptions =
      std::get<1>(vertex_input).data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = kVkPrimitiveType[primitive];
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = nullptr;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = nullptr;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamic_states;
  dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
  dynamic_state_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_create_info.pNext = nullptr;
  dynamic_state_create_info.flags = 0;
  dynamic_state_create_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_create_info.pDynamicStates = dynamic_states.data();

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shaderStages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state_create_info;
  pipeline_info.layout = shader->pipeline_layout;
  pipeline_info.renderPass = context_.GetRenderPass();
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info,
                                nullptr, &shader->pipeline) != VK_SUCCESS)
    DLOG << "failed to create graphics pipeline.";

  vkDestroyShaderModule(device_, frag_shader_module, nullptr);
  vkDestroyShaderModule(device_, vert_shader_module, nullptr);
}

void RendererVulkan::DestroyShader(std::shared_ptr<void> impl_data) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  frames_[current_frame_].pipelines_to_destroy.push_back(
      std::make_tuple(shader->pipeline, shader->pipeline_layout));
  *shader = {};
}

void RendererVulkan::ActivateShader(std::shared_ptr<void> impl_data) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  if (active_pipeline_ != shader->pipeline) {
    active_pipeline_ = shader->pipeline;
    vkCmdBindPipeline(frames_[current_frame_].draw_command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
  }
  for (int i = 0; i < shader->desc_set_count; ++i) {
    if (active_descriptor_sets_[i] != penging_descriptor_sets_[i]) {
      active_descriptor_sets_[i] = penging_descriptor_sets_[i];
      vkCmdBindDescriptorSets(frames_[current_frame_].draw_command_buffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              shader->pipeline_layout, 0, 1,
                              &active_descriptor_sets_[i], 0, nullptr);
      break;
    }
  }
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector2& val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector3& val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Vector4& val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                const base::Matrix4x4& val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                float val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::SetUniform(std::shared_ptr<void> impl_data,
                                const std::string& name,
                                int val) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  for (auto& sampler_name : shader->sampler_uniform_names) {
    if (name == sampler_name)
      return;
  }
  SetUniformInternal(shader, name, val);
}

void RendererVulkan::UploadUniforms(std::shared_ptr<void> impl_data) {
  auto shader = reinterpret_cast<ShaderVulkan*>(impl_data.get());
  vkCmdPushConstants(
      frames_[current_frame_].draw_command_buffer, shader->pipeline_layout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
      shader->push_constants_size, shader->push_constants.get());
}

void RendererVulkan::PrepareForDrawing() {
  context_.PrepareBuffers();
  DrawListBegin();
}

void RendererVulkan::Present() {
  DrawListEnd();
  SwapBuffers();
}

bool RendererVulkan::InitializeInternal() {
  glslang::InitializeProcess();

  device_ = context_.GetDevice();

  // Allocate one extra frame to ensure it's unused at any time without having
  // to use a fence.
  int frame_count = context_.GetSwapchainImageCount() + 1;
  frames_.resize(frame_count);
  frames_drawn_ = frame_count;

  // Initialize allocator
  VmaAllocatorCreateInfo allocator_info;
  memset(&allocator_info, 0, sizeof(VmaAllocatorCreateInfo));
  allocator_info.physicalDevice = context_.GetPhysicalDevice();
  allocator_info.device = device_;
  allocator_info.instance = context_.GetInstance();
  vmaCreateAllocator(&allocator_info, &allocator_);

  for (int i = 0; i < frames_.size(); i++) {
    // Create command pool, one per frame is recommended.
    VkCommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.queueFamilyIndex = context_.GetGraphicsQueue();
    cmd_pool_info.flags = 0;

    VkResult err = vkCreateCommandPool(device_, &cmd_pool_info, nullptr,
                                       &frames_[i].setup_command_pool);
    if (err) {
      DLOG << "vkCreateCommandPool failed with error " << std::to_string(err);
      return false;
    }

    err = vkCreateCommandPool(device_, &cmd_pool_info, nullptr,
                              &frames_[i].draw_command_pool);
    if (err) {
      DLOG << "vkCreateCommandPool failed with error " << std::to_string(err);
      return false;
    }

    // Create command buffers.
    VkCommandBufferAllocateInfo cmdbuf_info;
    cmdbuf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbuf_info.pNext = nullptr;
    cmdbuf_info.commandPool = frames_[i].setup_command_pool;
    cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdbuf_info.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(device_, &cmdbuf_info,
                                   &frames_[i].setup_command_buffer);
    if (err) {
      DLOG << "vkAllocateCommandBuffers failed with error "
           << std::to_string(err);
      continue;
    }

    cmdbuf_info.commandPool = frames_[i].draw_command_pool;
    err = vkAllocateCommandBuffers(device_, &cmdbuf_info,
                                   &frames_[i].draw_command_buffer);
    if (err) {
      DLOG << "vkAllocateCommandBuffers failed with error "
           << std::to_string(err);
      continue;
    }
  }

  // In this simple engine we use only one descriptor set layout that is for
  // textures. We use push contants for everything else.
  VkDescriptorSetLayoutBinding ds_layout_binding;
  ds_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  ds_layout_binding.descriptorCount = 1;
  ds_layout_binding.binding = 0;
  ds_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  ds_layout_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo ds_layout_info;
  ds_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  ds_layout_info.pNext = nullptr;
  ds_layout_info.flags = 0;
  ds_layout_info.bindingCount = 1;
  ds_layout_info.pBindings = &ds_layout_binding;

  VkResult res = vkCreateDescriptorSetLayout(device_, &ds_layout_info, nullptr,
                                             &descriptor_set_layout_);
  if (res) {
    DLOG << "Error (" << std::to_string(res)
         << ") creating descriptor set layout for set";
    return false;
  }

  // Create sampler.
  VkSamplerCreateInfo sampler_info;
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.pNext = nullptr;
  sampler_info.flags = 0;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.mipLodBias = 0;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = 0;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.minLod = 0;
  sampler_info.maxLod = 0;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;

  res = vkCreateSampler(device_, &sampler_info, nullptr, &sampler_);
  if (res) {
    DLOG << "vkCreateSampler failed with error " << std::to_string(res);
    return false;
  }

  texture_compression_.dxt1 = IsFormatSupported(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
  texture_compression_.s3tc = IsFormatSupported(VK_FORMAT_BC3_UNORM_BLOCK);

  LOG << "TextureCompression:";
  LOG << "  atc:   " << texture_compression_.atc;
  LOG << "  dxt1:  " << texture_compression_.dxt1;
  LOG << "  etc1:  " << texture_compression_.etc1;
  LOG << "  s3tc:  " << texture_compression_.s3tc;

  // Use a background thread for filling up staging buffers and recording setup
  // commands.
  quit_.store(false, std::memory_order_relaxed);
  setup_thread_ =
      std::thread(&RendererVulkan::SetupThreadMain, this, frame_count);

  // Begin the first command buffer for the first frame.
  BeginFrame();

  if (context_lost_cb_) {
    LOG << "Context lost.";
    context_lost_cb_();
  }
  return true;
}

void RendererVulkan::Shutdown() {
  if (device_ == VK_NULL_HANDLE)
    return;

  LOG << "Shutting down renderer.";
  InvalidateAllResources();

  quit_.store(true, std::memory_order_relaxed);
  semaphore_.Release();
  setup_thread_.join();

  vkDeviceWaitIdle(device_);

  for (int i = 0; i < frames_.size(); ++i) {
    FreePendingResources(i);
    vkDestroyCommandPool(device_, frames_[i].setup_command_pool, nullptr);
    vkDestroyCommandPool(device_, frames_[i].draw_command_pool, nullptr);
  }

  vmaDestroyAllocator(allocator_);

  vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
  vkDestroySampler(device_, sampler_, nullptr);

  device_ = VK_NULL_HANDLE;
  frames_drawn_ = 0;
  frames_.clear();
  current_frame_ = 0;

  staging_buffers_.clear();
  current_staging_buffer_ = 0;
  staging_buffer_used_ = false;

  context_.DestroyWindow();
  context_.Shutdown();

  glslang::FinalizeProcess();
}

void RendererVulkan::BeginFrame() {
  FreePendingResources(current_frame_);

  context_.AppendCommandBuffer(frames_[current_frame_].setup_command_buffer);
  context_.AppendCommandBuffer(frames_[current_frame_].draw_command_buffer);

  task_runner_.PostTask(HERE, [&]() {
    vkResetCommandPool(device_, frames_[current_frame_].setup_command_pool, 0);

    VkCommandBufferBeginInfo cmdbuf_begin;
    cmdbuf_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbuf_begin.pNext = nullptr;
    cmdbuf_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbuf_begin.pInheritanceInfo = nullptr;
    VkResult err = vkBeginCommandBuffer(
        frames_[current_frame_].setup_command_buffer, &cmdbuf_begin);
    if (err) {
      DLOG << "vkBeginCommandBuffer failed with error " << std::to_string(err);
      return;
    }
  });
  semaphore_.Release();

  vkResetCommandPool(device_, frames_[current_frame_].draw_command_pool, 0);

  VkCommandBufferBeginInfo cmdbuf_begin;
  cmdbuf_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdbuf_begin.pNext = nullptr;
  cmdbuf_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cmdbuf_begin.pInheritanceInfo = nullptr;
  VkResult err = vkBeginCommandBuffer(
      frames_[current_frame_].draw_command_buffer, &cmdbuf_begin);
  if (err) {
    DLOG << "vkBeginCommandBuffer failed with error " << std::to_string(err);
    return;
  }

  // Advance current frame.
  frames_drawn_++;

  // Advance staging buffer if used.
  if (staging_buffer_used_) {
    current_staging_buffer_ =
        (current_staging_buffer_ + 1) % staging_buffers_.size();
    staging_buffer_used_ = false;
  }
}

void RendererVulkan::FlushSetupBuffer() {
  vkEndCommandBuffer(frames_[current_frame_].setup_command_buffer);

  context_.Flush(false);

  vkResetCommandPool(device_, frames_[current_frame_].setup_command_pool, 0);

  VkCommandBufferBeginInfo cmdbuf_begin;
  cmdbuf_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdbuf_begin.pNext = nullptr;
  cmdbuf_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cmdbuf_begin.pInheritanceInfo = nullptr;

  VkResult err = vkBeginCommandBuffer(
      frames_[current_frame_].setup_command_buffer, &cmdbuf_begin);
  if (err) {
    DLOG << "vkBeginCommandBuffer failed with error " << std::to_string(err);
    return;
  }
  context_.AppendCommandBuffer(frames_[current_frame_].setup_command_buffer,
                               true);
}

void RendererVulkan::FreePendingResources(int frame) {
  if (!frames_[frame].pipelines_to_destroy.empty()) {
    for (auto& pipeline : frames_[frame].pipelines_to_destroy) {
      vkDestroyPipeline(device_, std::get<0>(pipeline), nullptr);
      vkDestroyPipelineLayout(device_, std::get<1>(pipeline), nullptr);
    }
    frames_[frame].pipelines_to_destroy.clear();
  }

  if (!frames_[frame].images_to_destroy.empty()) {
    for (auto& image : frames_[frame].images_to_destroy) {
      auto [buffer, view] = image;
      vkDestroyImageView(device_, view, nullptr);
      vmaDestroyImage(allocator_, std::get<0>(buffer), std::get<1>(buffer));
    }
    frames_[frame].images_to_destroy.clear();
  }

  if (!frames_[frame].buffers_to_destroy.empty()) {
    for (auto& buffer : frames_[frame].buffers_to_destroy)
      vmaDestroyBuffer(allocator_, std::get<0>(buffer), std::get<1>(buffer));
    frames_[frame].buffers_to_destroy.clear();
  }

  if (!frames_[frame].desc_sets_to_destroy.empty()) {
    for (auto& desc_set : frames_[frame].desc_sets_to_destroy) {
      auto [set, pool] = desc_set;
      vkFreeDescriptorSets(device_, std::get<0>(*pool), 1, &set);
      FreeDescriptorPool(pool);
    }
    frames_[frame].desc_sets_to_destroy.clear();
  }
}

void RendererVulkan::MemoryBarrier(VkPipelineStageFlags src_stage_mask,
                                   VkPipelineStageFlags dst_stage_mask,
                                   VkAccessFlags src_access,
                                   VkAccessFlags dst_sccess) {
  VkMemoryBarrier mem_barrier;
  mem_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  mem_barrier.pNext = nullptr;
  mem_barrier.srcAccessMask = src_access;
  mem_barrier.dstAccessMask = dst_sccess;

  vkCmdPipelineBarrier(frames_[current_frame_].draw_command_buffer,
                       src_stage_mask, dst_stage_mask, 0, 1, &mem_barrier, 0,
                       nullptr, 0, nullptr);
}

void RendererVulkan::FullBarrier() {
  // Used for debug.
  MemoryBarrier(
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
          VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
          VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
          VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
          VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT);
}

bool RendererVulkan::AllocateStagingBuffer(uint32_t amount,
                                           uint32_t segment,
                                           uint32_t& alloc_offset,
                                           uint32_t& alloc_size) {
  alloc_size = amount;

  while (true) {
    alloc_offset = 0;

    // See if we can use the current block.
    if (staging_buffers_[current_staging_buffer_].frame_used == frames_drawn_) {
      // We used this block this frame, let's see if there is still room.
      uint32_t write_from =
          staging_buffers_[current_staging_buffer_].fill_amount;
      int32_t available_bytes =
          int32_t(staging_buffer_size_) - int32_t(write_from);

      if ((int32_t)amount < available_bytes) {
        // All will fit.
        alloc_offset = write_from;
      } else if (segment > 0 && available_bytes >= (int32_t)segment) {
        // All won't fit but at least we can fit a chunk.
        alloc_offset = write_from;
        alloc_size = available_bytes - (available_bytes % segment);
      } else {
        // Try next buffer.
        current_staging_buffer_ =
            (current_staging_buffer_ + 1) % staging_buffers_.size();

        if (staging_buffers_[current_staging_buffer_].frame_used ==
            frames_drawn_) {
          // We manage to fill all blocks possible in a single frame. Check if
          // we can insert a new block.
          if (staging_buffers_.size() * staging_buffer_size_ <
              max_staging_buffer_size_) {
            if (!InsertStagingBuffer())
              return false;

            // Claim for current frame.
            staging_buffers_[current_staging_buffer_].frame_used =
                frames_drawn_;
          } else {
            // All the staging buffers belong to this frame and we can't create
            // more. Flush setup buffer to make room.
            FlushSetupBuffer();

            // Clear the whole staging buffer.
            for (int i = 0; i < staging_buffers_.size(); i++) {
              staging_buffers_[i].frame_used = 0;
              staging_buffers_[i].fill_amount = 0;
            }
            // Claim for current frame.
            staging_buffers_[current_staging_buffer_].frame_used =
                frames_drawn_;
          }
        } else {
          // Block is not from current frame, so continue and try again.
          continue;
        }
      }
    } else if (staging_buffers_[current_staging_buffer_].frame_used <=
               frames_drawn_ - frames_.size()) {
      // This is an old block, which was already processed, let's reuse.
      staging_buffers_[current_staging_buffer_].frame_used = frames_drawn_;
      staging_buffers_[current_staging_buffer_].fill_amount = 0;
    } else if (staging_buffers_[current_staging_buffer_].frame_used >
               frames_drawn_ - frames_.size()) {
      // This block may still be in use, let's not touch it unless we have to.
      // Check if we can insert a new block.
      if (staging_buffers_.size() * staging_buffer_size_ <
          max_staging_buffer_size_) {
        if (!InsertStagingBuffer())
          return false;

        // Claim for current frame.
        staging_buffers_[current_staging_buffer_].frame_used = frames_drawn_;
      } else {
        // We are out of room and we can't create more. Ensure older frames are
        // executed.
        vkDeviceWaitIdle(device_);

        for (int i = 0; i < staging_buffers_.size(); i++) {
          // Clear all blocks but the ones from this frame.
          int block_idx =
              (i + current_staging_buffer_) % staging_buffers_.size();
          if (staging_buffers_[block_idx].frame_used == frames_drawn_) {
            break;  // We reached something from this frame, abort.
          }

          staging_buffers_[block_idx].frame_used = 0;
          staging_buffers_[block_idx].fill_amount = 0;
        }
        // Claim for current frame.
        staging_buffers_[current_staging_buffer_].frame_used = frames_drawn_;
      }
    }

    // Done.
    break;
  }

  staging_buffers_[current_staging_buffer_].fill_amount =
      alloc_offset + alloc_size;
  staging_buffer_used_ = true;
  return true;
}

bool RendererVulkan::InsertStagingBuffer() {
  VkBufferCreateInfo buffer_info;
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.flags = 0;
  buffer_info.size = staging_buffer_size_;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_info.queueFamilyIndexCount = 0;
  buffer_info.pQueueFamilyIndices = nullptr;

  VmaAllocationCreateInfo alloc_info;
  alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;  // Stay mapped.
  alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;         // CPU and coherent.
  alloc_info.requiredFlags = 0;
  alloc_info.preferredFlags = 0;
  alloc_info.memoryTypeBits = 0;
  alloc_info.pool = nullptr;
  alloc_info.pUserData = nullptr;

  StagingBuffer block;

  VkResult err = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
                                 &std::get<0>(block.buffer),
                                 &std::get<1>(block.buffer), &block.alloc_info);
  if (err) {
    DLOG << "vmaCreateBuffer failed with error " << std::to_string(err);
    return false;
  }

  block.frame_used = 0;
  block.fill_amount = 0;

  staging_buffers_.insert(staging_buffers_.begin() + current_staging_buffer_,
                          block);
  return true;
}

RendererVulkan::DescPool* RendererVulkan::AllocateDescriptorPool() {
  DescPool* selected_pool = nullptr;

  for (auto& dp : desc_pools_) {
    if (std::get<1>(*dp) < kMaxDescriptorsPerPool) {
      selected_pool = dp.get();
      break;
    }
  }

  if (!selected_pool) {
    VkDescriptorPoolSize sizes;
    sizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes.descriptorCount = kMaxDescriptorsPerPool;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = kMaxDescriptorsPerPool;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &sizes;

    VkDescriptorPool desc_pool;
    VkResult res = vkCreateDescriptorPool(device_, &descriptor_pool_create_info,
                                          nullptr, &desc_pool);
    if (res) {
      DLOG << "vkCreateDescriptorPool failed with error "
           << std::to_string(res);
      return VK_NULL_HANDLE;
    }

    auto pool = std::make_unique<DescPool>(std::make_tuple(desc_pool, 0));
    selected_pool = pool.get();
    desc_pools_.push_back(std::move(pool));
  }

  ++std::get<1>(*selected_pool);
  return selected_pool;
}

void RendererVulkan::FreeDescriptorPool(DescPool* desc_pool) {
  if (--std::get<1>(*desc_pool) == 0) {
    for (auto it = desc_pools_.begin(); it != desc_pools_.end(); ++it) {
      if (std::get<0>(**it) == std::get<0>(*desc_pool)) {
        vkDestroyDescriptorPool(device_, std::get<0>(*desc_pool), nullptr);
        desc_pools_.erase(it);
        return;
      }
    }
    NOTREACHED;
  }
}

bool RendererVulkan::AllocateBuffer(Buffer<VkBuffer>& buffer,
                                    uint32_t size,
                                    uint32_t usage,
                                    VmaMemoryUsage mapping) {
  VkBufferCreateInfo buffer_info;
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.flags = 0;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_info.queueFamilyIndexCount = 0;
  buffer_info.pQueueFamilyIndices = nullptr;

  VmaAllocationCreateInfo allocation_info;
  allocation_info.flags = 0;
  allocation_info.usage = mapping;
  allocation_info.requiredFlags = 0;
  allocation_info.preferredFlags = 0;
  allocation_info.memoryTypeBits = 0;
  allocation_info.pool = nullptr;
  allocation_info.pUserData = nullptr;

  VkBuffer vk_buffer;
  VmaAllocation allocation = nullptr;

  VkResult err = vmaCreateBuffer(allocator_, &buffer_info, &allocation_info,
                                 &vk_buffer, &allocation, nullptr);
  if (err) {
    DLOG << "Can't create buffer of size: " << std::to_string(size)
         << ", error " << std::to_string(err);
    return false;
  }

  buffer = std::make_tuple(vk_buffer, allocation);

  return true;
}

void RendererVulkan::FreeBuffer(Buffer<VkBuffer> buffer) {
  frames_[current_frame_].buffers_to_destroy.push_back(std::move(buffer));
}

void RendererVulkan::UpdateBuffer(VkBuffer buffer,
                                  size_t offset,
                                  const void* data,
                                  size_t data_size) {
  size_t to_submit = data_size;
  size_t submit_from = 0;

  while (to_submit > 0) {
    uint32_t write_offset;
    uint32_t write_amount;

    if (!AllocateStagingBuffer(
            std::min((uint32_t)to_submit, staging_buffer_size_), 32,
            write_offset, write_amount))
      return;
    Buffer<VkBuffer> staging_buffer =
        staging_buffers_[current_staging_buffer_].buffer;

    // Copy to staging buffer.
    void* data_ptr =
        staging_buffers_[current_staging_buffer_].alloc_info.pMappedData;
    memcpy(((uint8_t*)data_ptr) + write_offset, (char*)data + submit_from,
           write_amount);

    // Insert a command to copy to GPU buffer.
    VkBufferCopy region;
    region.srcOffset = write_offset;
    region.dstOffset = submit_from + offset;
    region.size = write_amount;

    vkCmdCopyBuffer(frames_[current_frame_].setup_command_buffer,
                    std::get<0>(staging_buffer), buffer, 1, &region);

    to_submit -= write_amount;
    submit_from += write_amount;
  }
}

void RendererVulkan::BufferMemoryBarrier(VkBuffer buffer,
                                         uint64_t from,
                                         uint64_t size,
                                         VkPipelineStageFlags src_stage_mask,
                                         VkPipelineStageFlags dst_stage_mask,
                                         VkAccessFlags src_access,
                                         VkAccessFlags dst_sccess) {
  VkBufferMemoryBarrier buffer_mem_barrier;
  buffer_mem_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  buffer_mem_barrier.pNext = nullptr;
  buffer_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  buffer_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  buffer_mem_barrier.srcAccessMask = src_access;
  buffer_mem_barrier.dstAccessMask = dst_sccess;
  buffer_mem_barrier.buffer = buffer;
  buffer_mem_barrier.offset = from;
  buffer_mem_barrier.size = size;

  vkCmdPipelineBarrier(frames_[current_frame_].setup_command_buffer,
                       src_stage_mask, dst_stage_mask, 0, 0, nullptr, 1,
                       &buffer_mem_barrier, 0, nullptr);
}

bool RendererVulkan::CreateTexture(Buffer<VkImage>& image,
                                   VkImageView& view,
                                   DescSet& desc_set,
                                   VkFormat format,
                                   int width,
                                   int height,
                                   VkImageUsageFlags usage,
                                   VmaMemoryUsage mapping) {
  VkImageCreateInfo image_create_info;
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = nullptr;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.format = format;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_create_info.usage = usage;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = nullptr;

  VmaAllocationCreateInfo allocInfo;
  allocInfo.flags = 0;
  allocInfo.usage = mapping;
  allocInfo.requiredFlags = 0;
  allocInfo.preferredFlags = 0;
  allocInfo.memoryTypeBits = 0;
  allocInfo.pool = nullptr;
  allocInfo.pUserData = nullptr;

  VkImage vk_image;
  VmaAllocation allocation = nullptr;

  VkResult err = vmaCreateImage(allocator_, &image_create_info, &allocInfo,
                                &vk_image, &allocation, nullptr);
  if (err) {
    DLOG << "vmaCreateImage failed with error " << std::to_string(err);
    return false;
  }

  VkImageViewCreateInfo image_view_create_info;
  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_create_info.pNext = nullptr;
  image_view_create_info.flags = 0;
  image_view_create_info.image = vk_image;
  image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_create_info.format = format;
  image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.subresourceRange.baseMipLevel = 0;
  image_view_create_info.subresourceRange.levelCount = 1;
  image_view_create_info.subresourceRange.baseArrayLayer = 0;
  image_view_create_info.subresourceRange.layerCount = 1;
  image_view_create_info.subresourceRange.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;

  err = vkCreateImageView(device_, &image_view_create_info, nullptr, &view);

  if (err) {
    vmaDestroyImage(allocator_, vk_image, allocation);
    DLOG << "vkCreateImageView failed with error " << std::to_string(err);
    return false;
  }

  image = {vk_image, allocation};

  DescPool* desc_pool = AllocateDescriptorPool();

  VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
  descriptor_set_allocate_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_allocate_info.pNext = nullptr;
  descriptor_set_allocate_info.descriptorPool = std::get<0>(*desc_pool);
  descriptor_set_allocate_info.descriptorSetCount = 1;
  descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout_;

  VkDescriptorSet descriptor_set;
  VkResult res = vkAllocateDescriptorSets(
      device_, &descriptor_set_allocate_info, &descriptor_set);
  if (res) {
    --std::get<1>(*desc_pool);
    DLOG << "Cannot allocate descriptor sets, error " << std::to_string(res);
    return false;
  }

  desc_set = {descriptor_set, desc_pool};

  VkDescriptorImageInfo image_info;
  image_info.sampler = sampler_;
  image_info.imageView = view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write;
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;
  write.dstSet = descriptor_set;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &image_info;
  write.pBufferInfo = nullptr;
  write.pTexelBufferView = nullptr;

  vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

  return true;
}

void RendererVulkan::FreeTexture(Buffer<VkImage> image,
                                 VkImageView image_view,
                                 DescSet desc_set) {
  frames_[current_frame_].images_to_destroy.push_back(
      std::make_tuple(std::move(image), image_view));
  frames_[current_frame_].desc_sets_to_destroy.push_back(std::move(desc_set));
}

void RendererVulkan::UpdateImage(VkImage image,
                                 VkFormat format,
                                 const uint8_t* data,
                                 int width,
                                 int height) {
  int num_blocks_x = width;
  int num_blocks_y = height;
  GetNumBlocksForImageFormat(format, num_blocks_x, num_blocks_y);

  int block_size, block_height;
  GetBlockSizeForImageFormat(format, block_size, block_height);

  size_t to_submit = num_blocks_x * num_blocks_y * block_size;
  size_t submit_from = 0;
  uint32_t segment = num_blocks_x * block_size;
  uint32_t max_size = staging_buffer_size_ - (staging_buffer_size_ % segment);
  uint32_t region_offset_y = 0;

  // A segment must fit in a single staging buffer.
  DCHECK(staging_buffer_size_ >= segment);

  while (to_submit > 0) {
    uint32_t write_offset;
    uint32_t write_amount;
    if (!AllocateStagingBuffer(std::min((uint32_t)to_submit, max_size), segment,
                               write_offset, write_amount))
      return;
    Buffer<VkBuffer> staging_buffer =
        staging_buffers_[current_staging_buffer_].buffer;

    // Copy to staging buffer.
    void* data_ptr =
        staging_buffers_[current_staging_buffer_].alloc_info.pMappedData;
    memcpy(((uint8_t*)data_ptr) + write_offset, (char*)data + submit_from,
           write_amount);

    uint32_t region_height = (write_amount / segment) * block_height;

    // Insert a command to copy to GPU buffer.
    VkBufferImageCopy buffer_image_copy;
    buffer_image_copy.bufferOffset = write_offset;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = region_offset_y;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = width;
    buffer_image_copy.imageExtent.height = region_height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(frames_[current_frame_].setup_command_buffer,
                           std::get<0>(staging_buffer), image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &buffer_image_copy);

    to_submit -= write_amount;
    submit_from += write_amount;
    region_offset_y += region_height;
  }
}

void RendererVulkan::ImageMemoryBarrier(VkImage image,
                                        VkPipelineStageFlags src_stage_mask,
                                        VkPipelineStageFlags dst_stage_mask,
                                        VkAccessFlags src_access,
                                        VkAccessFlags dst_sccess,
                                        VkImageLayout old_layout,
                                        VkImageLayout new_layout) {
  VkImageMemoryBarrier image_mem_barrier;
  image_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_mem_barrier.pNext = nullptr;
  image_mem_barrier.srcAccessMask = src_access;
  image_mem_barrier.dstAccessMask = dst_sccess;
  image_mem_barrier.oldLayout = old_layout;
  image_mem_barrier.newLayout = new_layout;
  image_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_mem_barrier.image = image;
  image_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_mem_barrier.subresourceRange.baseMipLevel = 0;
  image_mem_barrier.subresourceRange.levelCount = 1;
  image_mem_barrier.subresourceRange.baseArrayLayer = 0;
  image_mem_barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(frames_[current_frame_].setup_command_buffer,
                       src_stage_mask, dst_stage_mask, 0, 0, nullptr, 0,
                       nullptr, 1, &image_mem_barrier);
}

bool RendererVulkan::CreatePipelineLayout(
    ShaderVulkan* shader,
    const std::vector<uint8_t>& spirv_vertex,
    const std::vector<uint8_t>& spirv_fragment) {
  SpvReflectShaderModule module_vertex;
  SpvReflectResult result = spvReflectCreateShaderModule(
      spirv_vertex.size(), spirv_vertex.data(), &module_vertex);
  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    DLOG << "SPIR-V reflection failed to parse vertex shader.";
    return false;
  }

  SpvReflectShaderModule module_fragment;
  result = spvReflectCreateShaderModule(
      spirv_fragment.size(), spirv_fragment.data(), &module_fragment);
  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    DLOG << "SPIR-V reflection failed to parse fragment shader.";
    spvReflectDestroyShaderModule(&module_vertex);
    return false;
  }

  bool ret = false;

  // Parse descriptor bindings.
  do {
    uint32_t binding_count = 0;

    // Validate that the vertex shader has no descriptor binding.
    result = spvReflectEnumerateDescriptorBindings(&module_vertex,
                                                   &binding_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
      DLOG << "SPIR-V reflection failed to enumerate fragment shader "
              "descriptor bindings.";
      break;
    }
    if (binding_count > 0) {
      DLOG << "SPIR-V reflection found " << binding_count
           << " descriptor bindings in vertex shader.";
      break;
    }

    // Validate that the fragment shader has max 1 desriptor binding.
    result = spvReflectEnumerateDescriptorBindings(&module_fragment,
                                                   &binding_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
      DLOG << "SPIR-V reflection failed to enumerate fragment shader "
              "descriptor bindings.";
      break;
    }

    DLOG << __func__ << " binding_count: " << binding_count;

    if (binding_count > 0) {
      // Collect binding names and validate that only COMBINED_IMAGE_SAMPLER
      // desriptor type is used.
      std::vector<SpvReflectDescriptorBinding*> bindings;
      bindings.resize(binding_count);
      result = spvReflectEnumerateDescriptorBindings(
          &module_fragment, &binding_count, bindings.data());

      if (result != SPV_REFLECT_RESULT_SUCCESS) {
        DLOG << "SPIR-V reflection failed to get descriptor bindings for "
                "fragment shader.";
        break;
      }

      for (int i = 0; i < binding_count; ++i) {
        const SpvReflectDescriptorBinding& binding = *bindings[i];

        DLOG << __func__ << " name: " << binding.name
             << " descriptor_type: " << binding.descriptor_type
             << " set: " << binding.set << " binding: " << binding.binding;

        if (binding.binding > 0) {
          DLOG << "SPIR-V reflection found " << binding_count
               << " bindings in vertex shader. Only one binding per set is "
                  "supported";
          break;
        }

        if (binding.descriptor_type !=
            SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          DLOG << "SPIR-V reflection found descriptor type "
               << binding.descriptor_type
               << " in fragment shader. Only COMBINED_IMAGE_SAMPLER type is "
                  "supported.";
          break;
        }

        shader->sampler_uniform_names.push_back(binding.name);
        shader->desc_set_count++;
      }
    }

    if (active_descriptor_sets_.size() < shader->desc_set_count) {
      active_descriptor_sets_.resize(shader->desc_set_count);
      penging_descriptor_sets_.resize(shader->desc_set_count);
    }

    // Parse push constants.
    auto enumerate_pc = [&](SpvReflectShaderModule& module, uint32_t& pc_count,
                            std::vector<SpvReflectBlockVariable*>& pconstants,
                            EShLanguage stage) {
      result =
          spvReflectEnumeratePushConstantBlocks(&module, &pc_count, nullptr);
      if (result != SPV_REFLECT_RESULT_SUCCESS) {
        DLOG << "SPIR-V reflection failed to enumerate push constats in shader "
                "stage "
             << stage;
        return false;
      }

      if (pc_count) {
        if (pc_count > 1) {
          DLOG << "SPIR-V reflection found " << pc_count
               << " push constats blocks in shader stage " << stage
               << ". Only one push constant block is supported.";
          return false;
        }

        pconstants.resize(pc_count);
        result = spvReflectEnumeratePushConstantBlocks(&module, &pc_count,
                                                       pconstants.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
          DLOG << "SPIR-V reflection failed to obtaining push constants.";
          return false;
        }
      }

      return true;
    };

    uint32_t pc_count_vertex = 0;
    std::vector<SpvReflectBlockVariable*> pconstants_vertex;
    if (!enumerate_pc(module_vertex, pc_count_vertex, pconstants_vertex,
                      EShLangVertex))
      break;

    uint32_t pc_count_fragment = 0;
    std::vector<SpvReflectBlockVariable*> pconstants_fragment;
    if (!enumerate_pc(module_fragment, pc_count_fragment, pconstants_fragment,
                      EShLangVertex))
      break;

    if (pc_count_vertex != pc_count_fragment) {
      DLOG << "SPIR-V reflection found different push constant blocks across "
              "shader stages.";
      break;
    }

    if (pc_count_vertex) {
      DLOG << __func__ << " PushConstants size: " << pconstants_vertex[0]->size
           << " count: " << pconstants_vertex[0]->member_count;

      if (pconstants_vertex[0]->size != pconstants_fragment[0]->size) {
        DLOG << "SPIR-V reflection found different push constant blocks across "
                "shader stages.";
        break;
      }

      shader->push_constants_size = pconstants_vertex[0]->size;
      shader->push_constants =
          std::make_unique<char[]>(shader->push_constants_size);
      memset(shader->push_constants.get(), 0, shader->push_constants_size);

      size_t offset = 0;
      for (uint32_t j = 0; j < pconstants_vertex[0]->member_count; j++) {
        DLOG << __func__ << " name: " << pconstants_vertex[0]->members[j].name
             << " size: " << pconstants_vertex[0]->members[j].size
             << " padded_size: "
             << pconstants_vertex[0]->members[j].padded_size;

        shader->variables[pconstants_vertex[0]->members[j].name] = {
            pconstants_vertex[0]->members[j].size, offset};
        offset += pconstants_vertex[0]->members[j].padded_size;
      }
    }

    // Use the same layout for all decriptor sets.
    std::vector<VkDescriptorSetLayout> desc_set_layouts;
    for (int i = 0; i < binding_count; ++i)
      desc_set_layouts.push_back(descriptor_set_layout_);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.flags = 0;
    if (binding_count > 0) {
      pipeline_layout_create_info.setLayoutCount = binding_count;
      pipeline_layout_create_info.pSetLayouts = desc_set_layouts.data();
    } else {
      pipeline_layout_create_info.setLayoutCount = 0;
      pipeline_layout_create_info.pSetLayouts = nullptr;
    }

    VkPushConstantRange push_constant_range;
    if (shader->push_constants_size) {
      push_constant_range.stageFlags =
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      push_constant_range.offset = 0;
      push_constant_range.size = shader->push_constants_size;

      pipeline_layout_create_info.pushConstantRangeCount = 1;
      pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
    } else {
      pipeline_layout_create_info.pushConstantRangeCount = 0;
      pipeline_layout_create_info.pPushConstantRanges = nullptr;
    }

    if (vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr,
                               &shader->pipeline_layout) != VK_SUCCESS) {
      DLOG << "Failed to create pipeline layout!";
      break;
    }

    ret = true;
  } while (false);

  spvReflectDestroyShaderModule(&module_vertex);
  spvReflectDestroyShaderModule(&module_fragment);
  return ret;
}

void RendererVulkan::DrawListBegin() {
  VkCommandBuffer command_buffer = frames_[current_frame_].draw_command_buffer;

  VkRenderPassBeginInfo render_pass_begin;
  render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin.pNext = nullptr;
  render_pass_begin.renderPass = context_.GetRenderPass();
  render_pass_begin.framebuffer = context_.GetFramebuffer();

  render_pass_begin.renderArea.extent.width = screen_width_;
  render_pass_begin.renderArea.extent.height = screen_height_;
  render_pass_begin.renderArea.offset.x = 0;
  render_pass_begin.renderArea.offset.y = 0;

  render_pass_begin.clearValueCount = 1;

  VkClearValue clear_value;
  clear_value.color.float32[0] = 0;
  clear_value.color.float32[1] = 0;
  clear_value.color.float32[2] = 0;
  clear_value.color.float32[3] = 1;

  render_pass_begin.pClearValues = &clear_value;

  vkCmdBeginRenderPass(command_buffer, &render_pass_begin,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport;
  viewport.x = 0;
  viewport.y = (float)screen_height_;
  viewport.width = (float)screen_width_;
  viewport.height = -(float)screen_height_;
  viewport.minDepth = 0;
  viewport.maxDepth = 1.0;

  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = screen_width_;
  scissor.extent.height = screen_height_;

  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void RendererVulkan::DrawListEnd() {
  vkCmdEndRenderPass(frames_[current_frame_].draw_command_buffer);

  // To ensure proper synchronization, we must make sure rendering is done
  // before:
  //  - Some buffer is copied.
  //  - Another render pass happens (since we may be done).
#ifdef FORCE_FULL_BARRIER
  FullBarrier(true);
#else
  MemoryBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                    VK_ACCESS_SHADER_WRITE_BIT);
#endif
}

void RendererVulkan::SwapBuffers() {
  // Ensure all tasks in the background thread are complete.
  task_runner_.WaitForCompletion();

  vkEndCommandBuffer(frames_[current_frame_].setup_command_buffer);
  vkEndCommandBuffer(frames_[current_frame_].draw_command_buffer);

  context_.SwapBuffers();
  current_frame_ = (current_frame_ + 1) % frames_.size();

  active_pipeline_ = VK_NULL_HANDLE;
  for (auto& ds : active_descriptor_sets_)
    ds = VK_NULL_HANDLE;
  for (auto& ds : penging_descriptor_sets_)
    ds = VK_NULL_HANDLE;

  BeginFrame();
}

void RendererVulkan::SetupThreadMain(int preallocate) {
  if (max_staging_buffer_size_ < staging_buffer_size_ * 4)
    max_staging_buffer_size_ = staging_buffer_size_ * 4;

  current_staging_buffer_ = 0;
  staging_buffer_used_ = false;

  for (int i = 0; i < preallocate; i++) {
    bool err = InsertStagingBuffer();
    LOG_IF(!err) << "Failed to create staging buffer.";
  }

  for (;;) {
    semaphore_.Acquire();
    if (quit_.load(std::memory_order_relaxed))
      break;

    task_runner_.SingleConsumerRun();
  }

  for (int i = 0; i < staging_buffers_.size(); i++) {
    auto [buffer, allocation] = staging_buffers_[i].buffer;
    vmaDestroyBuffer(allocator_, buffer, allocation);
  }
}

template <typename T>
bool RendererVulkan::SetUniformInternal(ShaderVulkan* shader,
                                        const std::string& name,
                                        T val) {
  auto it = shader->variables.find(name);
  if (it == shader->variables.end()) {
    DLOG << "No variable found with name " << name;
    return false;
  }
  if (it->second[0] != sizeof(val)) {
    DLOG << "Size mismatch for variable " << name;
    return false;
  }

  auto* dst =
      reinterpret_cast<T*>(shader->push_constants.get() + it->second[1]);
  *dst = val;
  return true;
}

bool RendererVulkan::IsFormatSupported(VkFormat format) {
  VkFormatProperties properties;
  vkGetPhysicalDeviceFormatProperties(context_.GetPhysicalDevice(), format,
                                      &properties);
  return properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
}

std::unique_ptr<RenderResource> RendererVulkan::CreateResource(
    RenderResourceFactoryBase& factory) {
  static unsigned last_id = 0;

  std::shared_ptr<void> impl_data;
  if (factory.IsTypeOf<Geometry>())
    impl_data = std::make_shared<GeometryVulkan>();
  else if (factory.IsTypeOf<Shader>())
    impl_data = std::make_shared<ShaderVulkan>();
  else if (factory.IsTypeOf<Texture>())
    impl_data = std::make_shared<TextureVulkan>();
  else
    NOTREACHED << "- Unknown resource type.";

  unsigned resource_id = ++last_id;
  auto resource = factory.Create(resource_id, impl_data, this);
  resources_[resource_id] = resource.get();
  return resource;
}

void RendererVulkan::ReleaseResource(unsigned resource_id) {
  auto it = resources_.find(resource_id);
  if (it != resources_.end())
    resources_.erase(it);
}

size_t RendererVulkan::GetAndResetFPS() {
  return context_.GetAndResetFPS();
}

void RendererVulkan::InvalidateAllResources() {
  for (auto& r : resources_)
    r.second->Destroy();
}

}  // namespace eng
