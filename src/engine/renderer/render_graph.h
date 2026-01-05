#ifndef ENGINE_RENDERER_RENDER_GRAPH_H
#define ENGINE_RENDERER_RENDER_GRAPH_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace eng {

class Renderer;

struct RenderGraphContext {
  Renderer* renderer;
};

class RenderPass {
 public:
  virtual ~RenderPass() = default;
  virtual void Execute(RenderGraphContext& ctx) = 0;
};

class LambdaRenderPass : public RenderPass {
 public:
  using Callback = std::function<void(RenderGraphContext&)>;
  explicit LambdaRenderPass(Callback callback) : callback_(std::move(callback)) {}
  void Execute(RenderGraphContext& ctx) final {
    if (callback_) callback_(ctx);
  }

 private:
  Callback callback_;
};

class RenderGraph {
 public:
  RenderGraph();
  ~RenderGraph();

  // Initializes the graph (creates composition resources).
  void Initialize(Renderer* renderer);

  // Adds a pass that renders into the specified 'layer_name'.
  // If 'layer_name' is empty, behavior is undefined (or could map to default).
  void AddPass(std::string name, std::string layer_name,
               std::unique_ptr<RenderPass> pass);

  void AddPass(std::string name, std::string layer_name,
               LambdaRenderPass::Callback callback);

  void Reset();

  void Execute(Renderer* renderer);

  uint64_t GetLayerTexture(const std::string& name);

 private:
  struct PassNode {
    std::string name;
    std::string layer_name;
    std::unique_ptr<RenderPass> pass;
  };

  struct RenderLayer {
    std::string name;
    uint64_t texture_id = 0;
    uint64_t descriptor_set_id = 0;
  };

  std::vector<PassNode> passes_;
  
  // Stores layers by name. We use a map to ensure unique layers.
  // We also keep a vector to preserve creation/composition order if needed.
  std::map<std::string, RenderLayer> layers_;
  std::vector<std::string> layer_order_;

  // Resources for the final composition pass
  uint64_t composite_shader_id_ = 0;
  uint64_t full_screen_quad_id_ = 0;
  bool initialized_ = false;

  RenderLayer& GetOrCreateLayer(Renderer* renderer, const std::string& name);
  void CreateCompositeResources(Renderer* renderer);
};

}  // namespace eng

#endif  // ENGINE_RENDERER_RENDER_GRAPH_H
