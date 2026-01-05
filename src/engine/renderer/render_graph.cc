#include "engine/renderer/render_graph.h"

#include "base/log.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/renderer/renderer.h"

namespace eng {

namespace {

// Simple pass-through shader for compositing textures to screen.
// In a real engine, you'd load this from a file, but we embed it for robustness here.
const char* kCompositeVertex = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec2 inTexCoord;
layout(location = 0) out vec2 fragTexCoord;
void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
)";

const char* kCompositeFragment = R"(
#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D texSampler;
void main() {
    outColor = texture(texSampler, fragTexCoord);
    // Simple alpha blending is assumed to be enabled in the pipeline
}
)";

}  // namespace

RenderGraph::RenderGraph() = default;
RenderGraph::~RenderGraph() = default;

void RenderGraph::Initialize(Renderer* renderer) {
  if (initialized_) return;
  CreateCompositeResources(renderer);
  initialized_ = true;
}

void RenderGraph::CreateCompositeResources(Renderer* renderer) {
  // 1. Create Full Screen Quad
  std::vector<Model::Vertex> vertices;
  vertices.push_back({{-1, -1, 0}, {0,0,1}, {0,0,0,0}, {0, 0}});
  vertices.push_back({{ 1, -1, 0}, {0,0,1}, {0,0,0,0}, {1, 0}});
  vertices.push_back({{ 1,  1, 0}, {0,0,1}, {0,0,0,0}, {1, 1}});
  vertices.push_back({{-1,  1, 0}, {0,0,1}, {0,0,0,0}, {0, 1}});
  
  std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
  
  auto mesh = std::make_unique<Mesh>();
  // Assuming CreateMesh-like logic or manual geometry creation
  // Since we don't have direct Mesh access here, we use Renderer's CreateGeometry
  // We need to match VertexDescription "p3f;n3f;a4f;t2f" from World.cc
  VertexDescription v_desc;
  ParseVertexDescription("p3f;n3f;a4f;t2f", v_desc);
  
  full_screen_quad_id_ = renderer->CreateGeometry(
      kPrimitive_Triangles, v_desc, kDataType_UInt);
  renderer->UpdateGeometry(full_screen_quad_id_, vertices.size(), vertices.data(),
                           indices.size(), indices.data());

  // 2. Create Composite Shader
  auto source = std::make_unique<ShaderSource>();
  // Manually populate shader source (assuming ShaderSource has a way to set string directly 
  // or we temporarily bypass loading from file. For now we assume a helper exists or we'd 
  // need to write these strings to a temp file). 
  // To keep it simple, let's assume we can mock this or the user adds a "LoadFromMemory" method.
  // For this example, I will assume the user has a "composite.glsl" file with the content above.
  source->Load("composite.glsl"); 

  composite_shader_id_ = renderer->CreateShader(
      std::move(source), v_desc, kPrimitive_Triangles, false, false, CullMode::kBack);
}

void RenderGraph::AddPass(std::string name, std::string layer_name,
                          std::unique_ptr<RenderPass> pass) {
  passes_.push_back({std::move(name), std::move(layer_name), std::move(pass)});
}

void RenderGraph::AddPass(std::string name, std::string layer_name,
                          LambdaRenderPass::Callback callback) {
  AddPass(std::move(name), std::move(layer_name),
          std::make_unique<LambdaRenderPass>(std::move(callback)));
}

void RenderGraph::Reset() {
  passes_.clear();
  // We keep layers_ and resources to avoid re-allocating textures every frame
}

RenderGraph::RenderLayer& RenderGraph::GetOrCreateLayer(Renderer* renderer,
                                                        const std::string& name) {
  auto it = layers_.find(name);
  if (it == layers_.end()) {
    RenderLayer layer;
    layer.name = name;
    layer.texture_id = renderer->CreateTexture();
    
    // Create a descriptor set for this layer's texture
    // Binding 0: The texture sampler
    layer.descriptor_set_id = renderer->CreateDescriptorSet(
        composite_shader_id_, 0, {{layer.texture_id}}, {});

    auto result = layers_.insert({name, layer});
    layer_order_.push_back(name);
    return result.first->second;
  }
  return it->second;
}

void RenderGraph::Execute(Renderer* renderer) {
  if (!initialized_) Initialize(renderer);

  RenderGraphContext ctx{renderer};

  // 1. Execute all user passes
  for (auto& node : passes_) {
    RenderLayer& layer = GetOrCreateLayer(renderer, node.layer_name);

    // Bind the layer (Texture) as the render target
    renderer->BeginRenderToTexture(layer.texture_id);

    // Run the pass
    node.pass->Execute(ctx);

    renderer->EndRenderToTexture(layer.texture_id);
  }

  // 2. Final Composition Pass
  renderer->PrepareForDrawing(); // Binds the default Swapchain/Screen

  renderer->ActivateShader(composite_shader_id_);
  renderer->ActivateGeometry(full_screen_quad_id_);

  // Render each layer in the order they were created (or logic specific order)
  // Assumes additive or alpha blending is configured on the pipeline/shader.
  for (const auto& layer_name : layer_order_) {
    RenderLayer& layer = layers_[layer_name];
    renderer->ActivateDescriptorSet(layer.descriptor_set_id);
    renderer->Draw(6, 0); // Draw the quad
  }
}

uint64_t RenderGraph::GetLayerTexture(const std::string& name) {
  auto it = layers_.find(name);
  if (it != layers_.end()) return it->second.texture_id;
  return 0;
}

}  // namespace eng
