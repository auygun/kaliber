#include "engine/model.h"

#include <iostream>
#include <span>
#include <sstream>

#include "base/log.h"
#include "engine/asset/image.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
#include "third_party/meshoptimizer/meshoptimizer.h"
#include "third_party/tiny_obj_loader/tiny_obj_loader.h"

using namespace base;

namespace eng {

namespace {

enum TextureUsage {
  kAlbedoMap,
  kNormalMap,
  kMetalnessMap,
  kRoughnessMap,
};

struct PushConstant {
  unsigned int material_index;
  unsigned int _pad0;
  unsigned int _pad1;
  unsigned int _pad2;
};

struct Vertex {
  Vector3f position{0};
  Vector3f normal{0};
  Vector3f tangent{0};
  float uv[2]{0, 0};
};

void GenerateTangents(std::vector<uint32_t> indices,
                      std::span<Vertex>& vertices) {
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
    Vector3f face_tangent = (edge1 * dv2 - edge2 * dv1) * r;

    // Accumulate face tangents for all three vertices
    v1.tangent += face_tangent;
    v2.tangent += face_tangent;
    v3.tangent += face_tangent;
  }

  // Normalize and orthogonalize tangent vectors
  for (Vertex& v : vertices) {
    v.tangent = v.tangent - v.normal * v.tangent.DotProduct(v.normal);
    v.tangent.Normalize();
  }
}

}  // namespace

bool Model::LoadObj(Renderer* renderer,
                    uint64_t shader_id,
                    VertexDescription vertex_description,
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

  auto mtl = AssetFile::ReadWholeFile(mtl_file_name.c_str(),
                                      Engine::Get().GetRootPath().c_str(),
                                      &buffer_size, true);
  if (!mtl) {
    LOG(0) << "Failed to read mtl file: " << file_name;
    return false;
  }
  std::istringstream mtl_stream(std::istringstream(mtl.get()));

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;
  tinyobj::MaterialStreamReader mtl_reader(mtl_stream);

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &obj_stream,
                        &mtl_reader)) {
    LOG(0) << "tinyobj::LoadObj failed";
    return false;
  }

  std::vector<Vertex> vertices;

  // Indices grouped by material
  std::unordered_map<int, std::vector<uint32_t>> material_indices;
  size_t total_index_count = 0;

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

        uint32_t new_index = vertices.size();
        vertices.push_back(vert);
        material_indices[material_id].push_back(new_index);
      }
      index_offset += fv;
    }
    total_index_count += index_offset;
  }

  DLOG(0) << "- Total vertices: " << vertices.size();

  // Deduplicate vertices.
  std::vector<uint32_t> remap(vertices.size());
  size_t total_vertices = meshopt_generateVertexRemap(
      remap.data(), nullptr, vertices.size(), vertices.data(), vertices.size(),
      sizeof(Vertex));

  std::vector<Vertex> unique_vertices(total_vertices);
  meshopt_remapVertexBuffer(unique_vertices.data(), vertices.data(),
                            vertices.size(), sizeof(Vertex), remap.data());

  DLOG(0) << "- Unique vertices: " << unique_vertices.size();

  std::vector<uint32_t> aggregated_indices(total_index_count);

  for (auto& mi : material_indices) {
    int material_id = mi.first;
    auto& indices = mi.second;

    if (material_id < 0 || material_id >= materials.size()) {
      LOG(0) << "Invalid material id: " << material_id;
      return false;
    }

    // Remap indices to unique vertices.
    std::vector<uint32_t> remapped_indices(indices.size());
    meshopt_remapIndexBuffer(remapped_indices.data(), indices.data(),
                             indices.size(), remap.data());

    // Optimize indices
    meshopt_optimizeVertexCache(
        remapped_indices.data(), remapped_indices.data(),
        remapped_indices.size(), unique_vertices.size());
    meshopt_optimizeOverdraw(remapped_indices.data(), remapped_indices.data(),
                             remapped_indices.size(),
                             &unique_vertices[0].position[0],
                             unique_vertices.size(), sizeof(Vertex), 1.05f);

    // Aggregate all indices into one index buffer.
    meshes_.push_back(
        {remapped_indices.size(),
         aggregated_indices.size(),
         {materials[material_id].diffuse[0], materials[material_id].diffuse[1],
          materials[material_id].diffuse[2]}});
    aggregated_indices.insert(aggregated_indices.end(),
                              remapped_indices.begin(), remapped_indices.end());
  }

  DLOG(0) << "- Meshes: " << meshes_.size();

  // Optimize vertices
  meshopt_optimizeVertexFetch(unique_vertices.data(), aggregated_indices.data(),
                              aggregated_indices.size(), unique_vertices.data(),
                              unique_vertices.size(), sizeof(Vertex));

#if 0
  for (auto& mesh : meshes_) {
    meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(
        &aggregated_indices[mesh.index_offset], mesh.num_indices,
        unique_vertices.size(), 16, 0, 0);
    DLOG(0) << "meshopt_analyzeVertexCache:";
    DLOG(0) << "- vertices_transformed: " << vcs.vertices_transformed;
    DLOG(0) << "- warps_executed      : " << vcs.warps_executed;
    DLOG(0) << "- acmr (0.5 - 3.0)    : " << vcs.acmr;
    DLOG(0) << "- atvr (1.0 - 6.0)    : " << vcs.atvr;

    meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(
        &aggregated_indices[mesh.index_offset], mesh.num_indices,
        unique_vertices.size(), sizeof(Vertex));
    DLOG(0) << "meshopt_analyzeVertexFetch:";
    DLOG(0) << "- bytes_fetched: " << vfs.bytes_fetched;
    DLOG(0) << "- overfetch    : " << vfs.overfetch;
  }
#endif

  std::span vertex_view(unique_vertices.data(), unique_vertices.size());
  GenerateTangents(aggregated_indices, vertex_view);

  // Create geometry
  geometry_.SetRenderer(renderer);
  geometry_.Create(kPrimitive_Triangles, vertex_description, kDataType_UInt);
  geometry_.Update(unique_vertices.size(), unique_vertices.data(),
                   aggregated_indices.size(), aggregated_indices.data());

  size_t index = 0;
  for (auto& file_name : texture_file_names) {
    bool is_srgb = index == kAlbedoMap;
    bool normalize = index == kNormalMap;
    LoadTexture(file_name, index, is_srgb, normalize);
    ++index;
  }

  materials_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_id, 2, 0, sizeof(MaterialData) * meshes_.size());
  materials_dset_ =
      renderer->CreateDescriptorSet(shader_id, 2,
                                    {{},
                                     {texture_[0].resource_id()},   // albedo
                                     {texture_[1].resource_id()},   // normal
                                     {texture_[2].resource_id()},   // metallic
                                     {texture_[3].resource_id()}},  // roughness
                                    {materials_ubo_});
  materials_.resize(meshes_.size());

  return true;
}

void Model::CreateMesh(Renderer* renderer,
                       uint64_t shader_id,
                       VertexDescription vertex_description,
                       std::vector<float> vertices,
                       std::vector<uint32_t> indices,
                       const std::vector<std::string>& texture_file_names) {
  renderer_ = renderer;

  std::span vertex_view(reinterpret_cast<Vertex*>(vertices.data()),
                        (sizeof(float) * vertices.size()) / sizeof(Vertex));
  GenerateTangents(indices, vertex_view);

  meshes_.push_back({indices.size(), 0, {1, 1, 1}});

  geometry_.SetRenderer(renderer);
  geometry_.Create(kPrimitive_Triangles, vertex_description, kDataType_UInt);
  geometry_.Update(vertices.size(), vertices.data(), indices.size(),
                   indices.data());

  size_t index = 0;
  for (auto& file_name : texture_file_names) {
    bool is_srgb = index == kAlbedoMap;
    bool normalize = index == kNormalMap;
    LoadTexture(file_name, index, is_srgb, normalize);
    ++index;
  }

  materials_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_id, 2, 0, sizeof(MaterialData) * meshes_.size());
  materials_dset_ =
      renderer->CreateDescriptorSet(shader_id, 2,
                                    {{},
                                     {texture_[0].resource_id()},   // albedo
                                     {texture_[1].resource_id()},   // normal
                                     {texture_[2].resource_id()},   // metallic
                                     {texture_[3].resource_id()}},  // roughness
                                    {materials_ubo_});
  materials_.resize(meshes_.size());
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

  texture_[index].SetRenderer(renderer_);
  texture_[index].Update(std::move(images));
}

void Model::Update(float metallic, float roughness, float ao) {
  for (int i = 0; i < meshes_.size(); ++i) {
    materials_[i].albedo = meshes_[i].color;
    materials_[i].metallic = metallic;
    materials_[i].roughness = roughness;
    materials_[i].ao = ao;
  }
  renderer_->UpdateBuffer(materials_ubo_, materials_.data(),
                          sizeof(MaterialData) * materials_.size());
}

void Model::Draw(unsigned int instance_count) {
  renderer_->ActivateDescriptorSet(materials_dset_);
  renderer_->ActivateGeometry(geometry_.resource_id());

  unsigned int material_index = 0;
  for (auto& mesh : meshes_) {
    PushConstant pc{};
    pc.material_index = material_index;
    renderer_->UpdatePushConstants(sizeof(pc), &pc);
    geometry_.Draw(mesh.num_indices, mesh.index_offset, instance_count, 0);
    ++material_index;
  }
}

}  // namespace eng
