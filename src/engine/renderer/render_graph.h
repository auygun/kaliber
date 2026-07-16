#ifndef ENGINE_RENDERER_RENDER_GRAPH_H
#define ENGINE_RENDERER_RENDER_GRAPH_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/renderer/renderer_types.h"

namespace eng {

class Renderer;

// Context passed to each pass during execution.
struct RenderGraphContext {
  Renderer* renderer;
};

// Base class for render passes. Each pass renders into an assigned render
// target (layer).
class RenderPass {
 public:
  virtual ~RenderPass() = default;
  virtual void Execute(RenderGraphContext& ctx) = 0;
};

// A render pass backed by a lambda/closure.
class LambdaRenderPass : public RenderPass {
 public:
  using Callback = std::function<void(RenderGraphContext&)>;
  explicit LambdaRenderPass(Callback callback)
      : callback_(std::move(callback)) {}

  void Execute(RenderGraphContext& ctx) final {
    if (callback_)
      callback_(ctx);
  }

 private:
  Callback callback_;
};

// A simple render graph that manages render targets (layers) and passes.
//
// Usage:
//   RenderGraph graph;
//   graph.Initialize(renderer);
//   graph.AddPass("ui", "ui_layer", [](RenderGraphContext& ctx) { ... });
//   graph.AddPass("scene", "scene_layer", [](RenderGraphContext& ctx) { ... });
//   graph.Execute(renderer);
//
// Each pass renders into its assigned layer. After all passes complete,
// the graph composites all layers to the screen.
class RenderGraph {
 public:
  RenderGraph();
  ~RenderGraph();

  // Initializes the graph (creates composition resources).
  // Must be called before Execute.
  void Initialize(Renderer* renderer);

  void ContextLost();

  // Adds a pass that renders into the specified 'layer_name'.
  // If the layer doesn't exist, it is created on first use.
  void AddPass(std::string name,
               std::string layer_name,
               std::unique_ptr<RenderPass> pass,
               bool depth = false);

  void AddPass(std::string name,
               std::string layer_name,
               LambdaRenderPass::Callback callback,
               bool depth = false);

  // Clears all passes but keeps layer resources alive for reuse.
  void Reset();

  // Executes all passes and composites the result to screen.
  void Execute(Renderer* renderer);

  // Returns the texture ID for a layer (for sampling in shaders).
  uint64_t GetLayerTexture(const std::string& name);

  // Returns the render target ID for a layer.
  uint64_t GetLayerRenderTarget(const std::string& name);

 private:
  struct PassNode {
    std::string name;
    std::string layer_name;
    std::unique_ptr<RenderPass> pass;
    bool depth = false;
  };

  struct RenderLayer {
    std::string name;
    uint64_t render_target_id = 0;
    uint64_t color_texture_id = 0;
    uint64_t descriptor_set_id = 0;
    int width = 0;
    int height = 0;
    bool depth = false;
  };

  std::vector<PassNode> passes_;

  // Stores layers by name. Map ensures unique layers.
  std::map<std::string, RenderLayer> layers_;
  std::vector<std::string> layer_order_;

  // Resources for the final composition pass.
  uint64_t composite_shader_id_ = 0;
  uint64_t full_screen_quad_id_ = 0;
  bool initialized_ = false;

  RenderLayer& GetOrCreateLayer(Renderer* renderer,
                                const std::string& name,
                                int width,
                                int height,
                                bool depth);
  void CreateCompositeResources(Renderer* renderer);
};

}  // namespace eng

#endif  // ENGINE_RENDERER_RENDER_GRAPH_H
