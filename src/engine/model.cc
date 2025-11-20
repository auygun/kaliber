#include "engine/model.h"

#include <iostream>
#include <sstream>

#include "base/log.h"
#include "engine/asset/image.h"
#include "engine/asset/mesh.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/renderer_types.h"
#include "third_party/meshoptimizer/meshoptimizer.h"
#include "third_party/tiny_obj_loader/tiny_obj_loader.h"
#include "third_party/tinygltf/tiny_gltf.h"

using namespace base;

// Implement tinygltf callbacks.
// These act as the default filesystem implementation for the loader.
namespace tinygltf {

bool FileExists(const std::string& abs_filename, void* user_data) {
  (void)user_data;
  size_t size = 0;
  // Check existence by attempting to read.
  auto data = eng::AssetFile::ReadWholeFile(
      abs_filename.c_str(), eng::Engine::Get().GetRootPath().c_str(), &size,
      true);
  return !!data;
}

std::string ExpandFilePath(const std::string& filepath, void* user_data) {
  (void)user_data;
#ifdef __ANDROID__
  // Android assets don't use absolute paths like /sdcard/, they are relative to
  // asset root.
  return filepath;
#else
  // On desktop, we might just return the path or handle relative paths if
  // needed. AssetFile handles concatenation with RootPath internally, so we
  // generally pass this through.
  return filepath;
#endif
}

bool ReadWholeFile(std::vector<unsigned char>* out,
                   std::string* err,
                   const std::string& filepath,
                   void* user_data) {
  (void)user_data;
  size_t size = 0;
  auto data = eng::AssetFile::ReadWholeFile(
      filepath.c_str(), eng::Engine::Get().GetRootPath().c_str(), &size, true);
  if (!data) {
    if (err)
      *err = "Failed to read asset file: " + filepath;
    return false;
  }

  out->resize(size);
  if (size > 0) {
    memcpy(out->data(), data.get(), size);
  }
  return true;
}

bool WriteWholeFile(std::string* err,
                    const std::string& filepath,
                    const std::vector<unsigned char>& contents,
                    void* user_data) {
  (void)filepath;
  (void)contents;
  (void)user_data;
  if (err)
    *err = "WriteWholeFile not supported for assets";
  return false;
}

bool GetFileSizeInBytes(size_t* filesize,
                        std::string* err,
                        const std::string& filepath,
                        void* user_data) {
  (void)user_data;
  size_t size = 0;
  // AssetFile doesn't expose a "PeekSize" yet, so we read to check.
  // Optimization: Add a GetSize method to AssetFile in the future.
  auto data = eng::AssetFile::ReadWholeFile(
      filepath.c_str(), eng::Engine::Get().GetRootPath().c_str(), &size, true);
  if (!data) {
    if (err)
      *err = "Failed to get file size: " + filepath;
    return false;
  }
  *filesize = size;
  return true;
}

}  // namespace tinygltf

namespace eng {

namespace {

const char vertex_description[] = "p3f;n3f;a4f;t2f";

enum TextureUsage {
  kAlbedoMap,
  kNormalMap,
  kMetalnessMap,
  kRoughnessMap,
};

struct PushConstant {
  unsigned int material_index;
  bool is_material;
  char _pad0[3];
  bool dir_light;
  char _pad1[3];
  bool cookie_cutter;
  char _pad2[3];
};

void Expand(base::Vector3f& min, base::Vector3f& max, const base::Vector3f& p) {
  min.x = std::min(min.x, p.x);
  min.y = std::min(min.y, p.y);
  min.z = std::min(min.z, p.z);
  max.x = std::max(max.x, p.x);
  max.y = std::max(max.y, p.y);
  max.z = std::max(max.z, p.z);
}

}  // namespace

Model::~Model() {
  renderer_->DestroyDescriptorSet(materials_dset_);
  renderer_->DestroyBuffer(materials_ubo_);
  for (int i = 0; i < std::size(texture_ids_); ++i)
    renderer_->DestroyTexture(texture_ids_[i]);
  renderer_->DestroyGeometry(geometry_id_);
}

bool Model::LoadObj(Renderer* renderer,
                    uint64_t shader_id,
                    const std::string& file_name,
                    const std::string& mtl_file_name,
                    const std::vector<std::string>& texture_file_names) {
  LOG(0) << "Loading " << file_name;

  renderer_ = renderer;

  size_t buffer_size = 0;
  auto obj = AssetFile::ReadWholeFile(file_name.c_str(),
                                      Engine::Get().GetRootPath().c_str(),
                                      &buffer_size, true);
  if (!obj) {
    LOG(0) << "Failed to read obj file: " << file_name;
    return false;
  }
  std::istringstream obj_stream(std::istringstream(obj.get()));

  std::unique_ptr<tinyobj::MaterialStreamReader> mtl_reader;
  std::istringstream mtl_stream;
  if (!mtl_file_name.empty()) {
    auto mtl = AssetFile::ReadWholeFile(mtl_file_name.c_str(),
                                        Engine::Get().GetRootPath().c_str(),
                                        &buffer_size, true);
    if (!mtl) {
      LOG(0) << "Failed to read mtl file: " << file_name;
      return false;
    }
    mtl_stream = std::istringstream(std::istringstream(mtl.get()));
    mtl_reader = std::make_unique<tinyobj::MaterialStreamReader>(mtl_stream);
  }

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &obj_stream,
                        mtl_reader.get())) {
    LOG(0) << "tinyobj::LoadObj failed";
    return false;
  }

  if (materials.empty()) {
    materials.push_back({});
    materials.back().diffuse[0] = 1;
    materials.back().diffuse[1] = 1;
    materials.back().diffuse[2] = 1;
  }

  std::vector<Vertex> raw_vertices;
  // Indices grouped by material
  std::unordered_map<int, std::vector<uint32_t>> material_indices;

  for (const auto& shape : shapes) {
    size_t index_offset = 0;
    for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
      int fv = shape.mesh.num_face_vertices[f];
      int material_id = shape.mesh.material_ids[f];

      for (int v = 0; v < fv; v++) {
        tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

        Vertex vert{};
        vert.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
        vert.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
        vert.position[2] = attrib.vertices[3 * idx.vertex_index + 2];

        if (idx.normal_index >= 0) {
          vert.normal[0] = attrib.normals[3 * idx.normal_index + 0];
          vert.normal[1] = attrib.normals[3 * idx.normal_index + 1];
          vert.normal[2] = attrib.normals[3 * idx.normal_index + 2];
        }

        if (idx.texcoord_index >= 0) {
          vert.uv[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
          vert.uv[1] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
        }

        uint32_t new_index = raw_vertices.size();
        raw_vertices.push_back(vert);
        material_indices[material_id].push_back(new_index);
      }
      index_offset += fv;
    }
  }

  std::vector<uint32_t> aggregated_indices;
  std::vector<size_t> material_indices_counts;

  for (auto& mi : material_indices) {
    int material_id = mi.first;
    auto& indices = mi.second;

    if (material_id < 0) {
      // No material. Use id 0.
      material_id = 0;
    } else if (material_id >= materials.size()) {
      LOG(0) << "Invalid material id: " << material_id;
      return false;
    }

    // Add the material for the sub-mesh.
    materials_.emplace_back(Vector4f{materials[material_id].diffuse[0],
                                     materials[material_id].diffuse[1],
                                     materials[material_id].diffuse[2], 1.0f},
                            1.0f, 0.3f, 0.5f);
    material_indices_counts.push_back(indices.size());
    // Aggregate all indices into one index buffer.
    aggregated_indices.insert(aggregated_indices.end(), indices.begin(),
                              indices.end());
  }

  DLOG(0) << "- Total raw vertices: " << raw_vertices.size();

  auto mesh =
      ProcessMesh(std::move(raw_vertices), std::move(aggregated_indices),
                  material_indices_counts, true);

  CreateRenderResources(shader_id, std::move(mesh), texture_file_names);

  return true;
}

bool Model::LoadGLTF(Renderer* renderer,
                     uint64_t shader_id,
                     const std::string& file_name) {
  LOG(0) << "Loading GLTF " << file_name;
  renderer_ = renderer;

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  size_t buffer_size = 0;
  auto gltf_data = AssetFile::ReadWholeFile(file_name.c_str(),
                                            Engine::Get().GetRootPath().c_str(),
                                            &buffer_size, true);
  if (!gltf_data) {
    LOG(0) << "Failed to read gltf file: " << file_name;
    return false;
  }

  std::string base_dir = "";
  size_t last_slash = file_name.find_last_of("/\\");
  if (last_slash != std::string::npos) {
    base_dir = file_name.substr(0, last_slash + 1);
  }

  bool ret = false;
  // Simple check for binary GLTF extension
  if (file_name.size() >= 4 &&
      file_name.substr(file_name.size() - 4) == ".glb") {
    ret = loader.LoadBinaryFromMemory(&model, &err, &warn,
                                      (unsigned char*)gltf_data.get(),
                                      buffer_size, base_dir);
  } else {
    ret = loader.LoadASCIIFromString(&model, &err, &warn, gltf_data.get(),
                                     buffer_size, base_dir);
  }

  if (!warn.empty()) {
    LOG(0) << "GLTF Warning: " << warn;
  }
  if (!err.empty()) {
    LOG(0) << "GLTF Error: " << err;
  }
  if (!ret) {
    LOG(0) << "Failed to parse GLTF";
    return false;
  }

  if (model.meshes.size() == 0) {
    LOG(0) << "GLTF file must contain a mesh.";
    return false;
  }

  const tinygltf::Mesh& mesh = model.meshes[0];
  std::vector<Vertex> raw_vertices;

  // Indices grouped by material ID (same logic as LoadObj)
  // Use std::map to ensure deterministic order if desired, though unordered is
  // fine too. Using int for material ID (-1 is default)
  std::map<int, std::vector<uint32_t>> material_indices_map;

  for (const auto& primitive : mesh.primitives) {
    // Validate indices exist
    if (primitive.indices < 0) {
      LOG(0) << "GLTF primitive must have indices.";
      return false;
    }

    const tinygltf::Accessor& index_accessor =
        model.accessors[primitive.indices];
    const tinygltf::BufferView& index_buffer_view =
        model.bufferViews[index_accessor.bufferView];
    const tinygltf::Buffer& index_buffer =
        model.buffers[index_buffer_view.buffer];

    const uint8_t* index_data_ptr = index_buffer.data.data() +
                                    index_buffer_view.byteOffset +
                                    index_accessor.byteOffset;

    size_t index_count = index_accessor.count;
    std::vector<uint32_t> indices;
    indices.reserve(index_count);

    // Helper to unpack indices
    if (index_accessor.componentType ==
        TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      const uint16_t* p = (const uint16_t*)index_data_ptr;
      for (size_t i = 0; i < index_count; ++i)
        indices.push_back(p[i]);
    } else if (index_accessor.componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      const uint32_t* p = (const uint32_t*)index_data_ptr;
      for (size_t i = 0; i < index_count; ++i)
        indices.push_back(p[i]);
    } else if (index_accessor.componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
      const uint8_t* p = (const uint8_t*)index_data_ptr;
      for (size_t i = 0; i < index_count; ++i)
        indices.push_back(p[i]);
    }

    // Attribute buffers
    const float* position_buffer = nullptr;
    const float* normal_buffer = nullptr;
    const float* tangent_buffer = nullptr;
    const float* texcoord_buffer = nullptr;

    int pos_stride = 0;
    int norm_stride = 0;
    int tan_stride = 0;
    int uv_stride = 0;

    size_t vertex_count = 0;

    // Find attributes
    for (auto& attrib : primitive.attributes) {
      const tinygltf::Accessor& accessor = model.accessors[attrib.second];
      const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
      const uint8_t* data = model.buffers[view.buffer].data.data() +
                            view.byteOffset + accessor.byteOffset;
      int stride = accessor.ByteStride(view);

      if (attrib.first == "POSITION") {
        position_buffer = (const float*)data;
        pos_stride = stride ? stride : 12;
        vertex_count = accessor.count;
      } else if (attrib.first == "NORMAL") {
        normal_buffer = (const float*)data;
        norm_stride = stride ? stride : 12;
      } else if (attrib.first == "TANGENT") {
        tangent_buffer = (const float*)data;
        tan_stride = stride ? stride : 16;
      } else if (attrib.first == "TEXCOORD_0") {
        texcoord_buffer = (const float*)data;
        uv_stride = stride ? stride : 8;
      }
    }

    if (!position_buffer) {
      LOG(0) << "GLTF primitive missing POSITION";
      return false;
    }

    uint32_t base_vertex_index = raw_vertices.size();
    for (size_t v = 0; v < vertex_count; ++v) {
      Vertex vert{};

      const float* pos =
          (const float*)((const uint8_t*)position_buffer + v * pos_stride);
      vert.position = base::Vector3f(pos[0], pos[1], pos[2]);

      if (normal_buffer) {
        const float* norm =
            (const float*)((const uint8_t*)normal_buffer + v * norm_stride);
        vert.normal = base::Vector3f(norm[0], norm[1], norm[2]);
      }

      // Load tangents directly from GLTF
      if (tangent_buffer) {
        const float* tan =
            (const float*)((const uint8_t*)tangent_buffer + v * tan_stride);
        vert.tangent = base::Vector4f(tan[0], tan[1], tan[2], tan[3]);
      }

      if (texcoord_buffer) {
        const float* uv =
            (const float*)((const uint8_t*)texcoord_buffer + v * uv_stride);
        vert.uv[0] = uv[0];
        // Flip V to match OpenGL/OBJ conventions used elsewhere in the engine
        vert.uv[1] = 1.0f - uv[1];
      }

      raw_vertices.push_back(vert);
    }

    // Group indices by material, applying the vertex offset
    auto& target_indices = material_indices_map[primitive.material];
    for (uint32_t idx : indices) {
      target_indices.push_back(base_vertex_index + idx);
    }
  }

  // Flatten indices and build materials list
  std::vector<uint32_t> aggregated_indices;
  std::vector<size_t> material_indices_counts;

  // Track indices of textures found in the first relevant material
  // (Current Model implementation supports one global set of textures)
  int found_albedo_idx = -1;
  int found_normal_idx = -1;
  int found_metal_rough_idx = -1;

  for (const auto& entry : material_indices_map) {
    int material_id = entry.first;
    const std::vector<uint32_t>& indices = entry.second;

    MaterialData mat_data;
    mat_data.albedo = base::Vector4f(1, 1, 1, 1);
    mat_data.metallic = 1.0f;
    mat_data.roughness = 1.0f;

    // Get material data if ID is valid
    if (material_id >= 0 && material_id < model.materials.size()) {
      const auto& mat = model.materials[material_id];
      const auto& pbr = mat.pbrMetallicRoughness;

      mat_data.albedo = base::Vector4f(
          (float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1],
          (float)pbr.baseColorFactor[2], (float)pbr.baseColorFactor[3]);
      mat_data.metallic = (float)pbr.metallicFactor;
      mat_data.roughness = (float)pbr.roughnessFactor;

      // Store texture indices from the first material we encounter that has
      // them
      if (found_albedo_idx == -1 && pbr.baseColorTexture.index != -1) {
        found_albedo_idx = model.textures[pbr.baseColorTexture.index].source;
      }
      if (found_metal_rough_idx == -1 &&
          pbr.metallicRoughnessTexture.index != -1) {
        found_metal_rough_idx =
            model.textures[pbr.metallicRoughnessTexture.index].source;
      }
      if (found_normal_idx == -1 && mat.normalTexture.index != -1) {
        found_normal_idx = model.textures[mat.normalTexture.index].source;
      }
    }

    materials_.push_back(mat_data);
    material_indices_counts.push_back(indices.size());
    aggregated_indices.insert(aggregated_indices.end(), indices.begin(),
                              indices.end());
  }

  // Process mesh with generate_tangents = false
  auto final_mesh =
      ProcessMesh(std::move(raw_vertices), std::move(aggregated_indices),
                  material_indices_counts, false);

  // Gather texture paths
  auto GetImageUri = [&](int index) -> std::string {
    if (index >= 0 && index < model.images.size()) {
      const auto& img = model.images[index];
      if (!img.uri.empty())
        return base_dir + img.uri;
    }
    return "";
  };

  std::vector<std::string> textures_to_load;
  std::string albedo_file = GetImageUri(found_albedo_idx);
  std::string normal_file = GetImageUri(found_normal_idx);
  std::string mr_file = GetImageUri(found_metal_rough_idx);

  // If any texture exists, we populate the list for CreateRenderResources.
  // Slot order: 0:Albedo, 1:Normal, 2:Metal, 3:Rough
  if (!albedo_file.empty() || !normal_file.empty() || !mr_file.empty()) {
    textures_to_load.push_back(albedo_file);
    textures_to_load.push_back(normal_file);
    textures_to_load.push_back(mr_file);
    // Metal/Roughness are packed in GLTF, so we use the same file for both
    // slots
    textures_to_load.push_back(mr_file);
  }

  CreateRenderResources(shader_id, std::move(final_mesh), textures_to_load);
  return true;
}

void Model::CreateMesh(Renderer* renderer,
                       uint64_t shader_id,
                       std::vector<Vertex> vertices,
                       std::vector<uint32_t> indices,
                       const std::vector<std::string>& texture_file_names) {
  renderer_ = renderer;

  size_t vertex_count = (sizeof(float) * vertices.size()) / sizeof(Vertex);
  DLOG(0) << "- Total vertices: " << vertex_count;
  DLOG(0) << "- Total indices: " << indices.size();

  std::vector<size_t> material_indices_counts;
  material_indices_counts.push_back(indices.size());
  auto mesh = ProcessMesh(std::move(vertices), std::move(indices),
                          material_indices_counts, true);

  CreateRenderResources(shader_id, std::move(mesh), texture_file_names);
}

std::unique_ptr<Mesh> Model::ProcessMesh(
    std::vector<Vertex> raw_vertices,
    std::vector<uint32_t> aggregated_indices,
    const std::vector<size_t>& material_indices_counts,
    bool generate_tangents) {
  // Deduplicate vertices.
  std::vector<uint32_t> remap(raw_vertices.size());
  size_t total_vertices = meshopt_generateVertexRemap(
      remap.data(), nullptr, raw_vertices.size(), raw_vertices.data(),
      raw_vertices.size(), sizeof(Vertex));

  std::vector<Vertex> unique_vertices(total_vertices);
  meshopt_remapVertexBuffer(unique_vertices.data(), raw_vertices.data(),
                            raw_vertices.size(), sizeof(Vertex), remap.data());

  DLOG(0) << "- Unique vertices: " << unique_vertices.size();

  // Calculate extents and center
  Vector3f min{0};
  Vector3f max{0};
  for (auto& v : unique_vertices)
    Expand(min, max, v.position);

  extents_ = (max - min) * 0.5f;
  DLOG(0) << "- extents: " << extents_.ToString();

  Vector3f center = (min + max) * 0.5f;
  for (auto& v : unique_vertices)
    v.position -= center;

  // Remap indices
  std::vector<uint32_t> remapped_indices(aggregated_indices.size());
  meshopt_remapIndexBuffer(remapped_indices.data(), aggregated_indices.data(),
                           aggregated_indices.size(), remap.data());

  // Process each material group
  size_t current_index_offset = 0;
  std::vector<uint32_t> final_indices;
  final_indices.reserve(remapped_indices.size());

  for (size_t i = 0; i < material_indices_counts.size(); ++i) {
    size_t count = material_indices_counts[i];
    if (count == 0)
      continue;

    uint32_t* indices_ptr = remapped_indices.data() + current_index_offset;

    // Optimize indices for cache
    meshopt_optimizeVertexCache(indices_ptr, indices_ptr, count,
                                unique_vertices.size());

    // Optimize overdraw
    meshopt_optimizeOverdraw(indices_ptr, indices_ptr, count,
                             &unique_vertices[0].position[0],
                             unique_vertices.size(), sizeof(Vertex), 1.05f);

    // Store draw command
    draw_list_.emplace_back(count, final_indices.size());

    // Append to final buffer
    final_indices.insert(final_indices.end(), indices_ptr, indices_ptr + count);

    current_index_offset += count;
  }

  DLOG(0) << "- draw_list_.size: " << draw_list_.size();

  // Optimize vertex fetch
  meshopt_optimizeVertexFetch(unique_vertices.data(), final_indices.data(),
                              final_indices.size(), unique_vertices.data(),
                              unique_vertices.size(), sizeof(Vertex));

#if 0
  for (auto& draw_cmd : draw_list_) {
    meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(
        &final_indices[draw_cmd.index_offset], draw_cmd.num_indices,
        unique_vertices.size(), 16, 0, 0);
    DLOG(0) << "meshopt_analyzeVertexCache:";
    DLOG(0) << "- vertices_transformed: " << vcs.vertices_transformed;
    DLOG(0) << "- warps_executed      : " << vcs.warps_executed;
    DLOG(0) << "- acmr (0.5 - 3.0)    : " << vcs.acmr;
    DLOG(0) << "- atvr (1.0 - 6.0)    : " << vcs.atvr;

    meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(
        &final_indices[draw_cmd.index_offset], draw_cmd.num_indices,
        unique_vertices.size(), sizeof(Vertex));
    DLOG(0) << "meshopt_analyzeVertexFetch:";
    DLOG(0) << "- bytes_fetched: " << vfs.bytes_fetched;
    DLOG(0) << "- overfetch    : " << vfs.overfetch;
  }
#endif

  if (generate_tangents)
    GenerateTangents(final_indices, unique_vertices);

  auto mesh = std::make_unique<Mesh>();
  mesh->Create(kPrimitive_Triangles, vertex_description, unique_vertices.size(),
               unique_vertices.data(), kDataType_UInt, final_indices.size(),
               final_indices.data());

  return mesh;
}

// Calculates tangent vectors per-vertex for a mesh, handling shared vertices by
// accumulating face tangents and then normalizing/orthogonalizing them.
//
// Limitation: This code forces the binormal direction to always be treated as
// right-handed (sets the w component to 1.0f). This is acceptable only if the
// given mesh doesn't use mirrored UV coordinates (e.g., simple run-time
// generated meshes like planes, spheres, or terrain).
void Model::GenerateTangents(const std::vector<uint32_t>& indices,
                             std::vector<Vertex>& vertices) {
  // Iterate over triangles. We sum the face tangents for shared vertices, then
  // normalize and orthogonalize later.
  for (size_t i = 0; i < indices.size(); i += 3) {
    // Get the three vertices of the current triangle
    Vertex& v1 = vertices[indices[i + 0]];
    Vertex& v2 = vertices[indices[i + 1]];
    Vertex& v3 = vertices[indices[i + 2]];

    // Position and UV differences
    Vector3f edge1 = v2.position - v1.position;
    Vector3f edge2 = v3.position - v1.position;

    float du1 = v2.uv[0] - v1.uv[0];
    float dv1 = v2.uv[1] - v1.uv[1];
    float du2 = v3.uv[0] - v1.uv[0];
    float dv2 = v3.uv[1] - v1.uv[1];

    // Calculate the determinant of the 2x2 UV matrix
    float det = du1 * dv2 - du2 * dv1;

    // Handle degenerate UV coordinates (where det is zero or near zero)
    if (std::abs(det) < 1e-6f)
      continue;

    float r = 1.0f / det;

    // Solve the linear system for the face tangent
    Vector4f face_tangent{(edge1 * dv2 - edge2 * dv1) * r, 0.0f};

    // Accumulate face tangents for all three vertices
    v1.tangent += face_tangent;
    v2.tangent += face_tangent;
    v3.tangent += face_tangent;
  }

  // Normalize and orthogonalize tangent vectors
  for (Vertex& v : vertices) {
    auto t = v.tangent.GetVector3();
    t = t - v.normal * t.DotProduct(v.normal);
    t.Normalize();
    v.tangent = Vector4f(t, 1.0f);
  }
}

void Model::CreateRenderResources(
    uint64_t shader_id,
    std::unique_ptr<Mesh> mesh,
    const std::vector<std::string>& texture_file_names) {
  // Create the geometry.
  geometry_id_ = renderer_->CreateGeometry(std::move(mesh));

  // Create a UBO for all materials.
  materials_ubo_ = renderer_->CreateBuffer(
      shader_id, 2, 0, sizeof(MaterialData) * materials_.size());
  renderer_->UpdateBuffer(materials_ubo_, materials_.data(),
                          sizeof(MaterialData) * materials_.size());

  // Create all textures and mipmaps.
  is_material_ = texture_file_names.empty();
  size_t index = 0;
  std::vector<std::vector<uint64_t>> textures(5);
  for (auto& file_name : texture_file_names) {
    if (!file_name.empty()) {
      bool is_srgb = index == kAlbedoMap;
      bool normalize = index == kNormalMap;
      LoadTexture(file_name, index, is_srgb, normalize);
      textures[index + 1].push_back(texture_ids_[index]);
    }
    ++index;
  }

  // Create the descriptor set.
  materials_dset_ =
      renderer_->CreateDescriptorSet(shader_id, 2, textures, {materials_ubo_});
}

void Model::LoadTexture(const std::string& file_name,
                        size_t index,
                        bool is_srgb,
                        bool normalize) {
  auto image = std::make_unique<Image>();
  if (!image->Load(file_name))
    return;
  if (is_srgb)
    image->SRGB2Linear();

  std::vector<std::unique_ptr<Image>> images;
  do {
    images.push_back(std::move(image));
    image = std::make_unique<Image>();
  } while (image->CreateMip(*images.back(), normalize));
  DLOG(0) << file_name << " mip levels: " << images.size();

  texture_ids_[index] = renderer_->CreateTexture();
  renderer_->UpdateTexture(texture_ids_[index], std::move(images));
}

void Model::Draw(unsigned int instance_count, unsigned int fist_instance) {
  renderer_->ActivateDescriptorSet(materials_dset_);
  renderer_->ActivateGeometry(geometry_id_);

  unsigned int material_index = 0;
  for (auto& draw_cmd : draw_list_) {
    PushConstant pc{};
    pc.material_index = material_index;
    pc.dir_light = true;
    pc.is_material = is_material_;
    pc.cookie_cutter = false;
    renderer_->UpdatePushConstants(sizeof(pc), &pc);
    renderer_->Draw(draw_cmd.num_indices, draw_cmd.index_offset, instance_count,
                    fist_instance);
    ++material_index;
  }
}

}  // namespace eng
