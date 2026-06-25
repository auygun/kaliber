#include "engine/renderer/render_graph.h"

#include <algorithm>

#include "base/log.h"
#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/model.h"
#include "engine/renderer/renderer.h"

namespace eng {

RenderGraph::RenderGraph() = default;

RenderGraph::~RenderGraph() = default;

void RenderGraph::Initialize(Renderer* renderer) {
  if (initialized_) return;
  CreateCompositeResources(renderer);
  initialized_ = true;
}

void RenderGraph::CreateCompositeResources(Renderer* renderer) {
  Model::Vertex vertices[] = {
      {{-1, -1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 1}},
      {{1, -1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {1, 1}},
      {{1, 1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {1, 0}},
      {{-1, 1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}},
  };
  uint32_t indices[] = {0, 1, 2, 2, 3, 0};

  auto mesh = std::make_unique<Mesh>();
  mesh->Create(kPrimitive_Triangles, "p3f;n3f;a4f;t2f", 4, vertices,
               kDataType_UInt, 6, indices);

  VertexDescription v_desc;
  ParseVertexDescription("p3f;n3f;a4f;t2f", v_desc);

  full_screen_quad_id_ = renderer->CreateGeometry(std::move(mesh));

  auto source = std::make_unique<ShaderSource>();
  if (!source->Load("engine/composite.glsl")) {
    LOG(0) << "Failed to load composite shader.";
    return;
  }

  composite_shader_id_ = renderer->CreateShader(std::move(source), v_desc,
                                                kPrimitive_Triangles, false,
                                                false, CullMode::kNone);
}

void RenderGraph::AddPass(std::string name, std::string layer_name,
                          std::unique_ptr<RenderPass> pass, bool depth) {
  passes_.push_back(
      {std::move(name), std::move(layer_name), std::move(pass), depth});
}

void RenderGraph::AddPass(std::string name, std::string layer_name,
                          LambdaRenderPass::Callback callback, bool depth) {
  AddPass(std::move(name), std::move(layer_name),
          std::make_unique<LambdaRenderPass>(std::move(callback)), depth);
}

void RenderGraph::Reset() {
  passes_.clear();
  // Keep layers_ and resources to avoid re-allocating textures every frame.
}

RenderGraph::RenderLayer& RenderGraph::GetOrCreateLayer(Renderer* renderer,
                                                        const std::string& name,
                                                        int width, int height,
                                                        bool depth) {
  auto it = layers_.find(name);
  if (it != layers_.end()) {
    if (it->second.width == width && it->second.height == height)
      return it->second;

    // Screen resized — destroy old resources and recreate below.
    renderer->DestroyDescriptorSet(it->second.descriptor_set_id);
    renderer->DestroyRenderTarget(it->second.render_target_id);
    layers_.erase(it);
    std::erase(layer_order_, name);
  }

  RenderLayer layer;
  layer.name = name;
  layer.width = width;
  layer.height = height;

  layer.depth = depth;
  layer.render_target_id =
      renderer->CreateRenderTarget(ImageFormat::kRGBA32, width, height, depth);

  // Get the color texture ID from the render target for sampling.
  layer.color_texture_id =
      renderer->GetRenderTargetColorTexture(layer.render_target_id);

  // Create a descriptor set for this layer's texture (for composition).
  // Set 0, Binding 0: The texture sampler.
  layer.descriptor_set_id = renderer->CreateDescriptorSet(
      composite_shader_id_, 0, {{layer.color_texture_id}}, {});

  auto result = layers_.insert({name, layer});
  layer_order_.push_back(name);
  return result.first->second;
}

void RenderGraph::Execute(Renderer* renderer) {
  if (!initialized_) Initialize(renderer);

  RenderGraphContext ctx{renderer};

  // Get screen dimensions for render target sizing.
  int screen_width = renderer->GetScreenWidth();
  int screen_height = renderer->GetScreenHeight();

  // 1. Execute all user passes, each rendering into its layer.
  for (auto& node : passes_) {
    RenderLayer& layer = GetOrCreateLayer(renderer, node.layer_name,
                                           screen_width, screen_height,
                                           node.depth);

    // Activate the render target and begin the render pass.
    renderer->ActivateRenderTarget(layer.render_target_id);

    node.pass->Execute(ctx);
  }

  // 2. End rendering to render targets and return to default framebuffer.
  renderer->EndRenderPassToDefault();

  // 3. Final Composition Pass - blend all layers to screen.
  renderer->ActivateShader(composite_shader_id_);
  renderer->ActivateGeometry(full_screen_quad_id_);

  for (const auto& layer_name : layer_order_) {
    const RenderLayer& layer = layers_[layer_name];
    if (layer.descriptor_set_id != 0) {
      renderer->ActivateDescriptorSet(layer.descriptor_set_id);
      renderer->Draw(6, 0);
    }
  }
}

uint64_t RenderGraph::GetLayerTexture(const std::string& name) {
  auto it = layers_.find(name);
  if (it != layers_.end()) return it->second.color_texture_id;
  return 0;
}

uint64_t RenderGraph::GetLayerRenderTarget(const std::string& name) {
  auto it = layers_.find(name);
  if (it != layers_.end()) return it->second.render_target_id;
  return 0;
}

}  // namespace eng
