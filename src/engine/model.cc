#include "engine/model.h"

#include <iostream>
#include <sstream>

#include "base/log.h"
#include "engine/asset/image.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/shader.h"
#include "third_party/meshoptimizer/meshoptimizer.h"
#include "third_party/tiny_obj_loader/tiny_obj_loader.h"

namespace eng {

namespace {

struct Vertex {
  float position[3];
  float normal[3];
  float uv[2];
};

const char vertex_description[] = "p3f;n3f;t2f";

}  // namespace

bool Model::LoadObj(Renderer* renderer,
                    const std::string& file_name,
                    const std::string& mtl_file_name,
                    const std::string& tex_file_name,
                    uint64_t shader_id) {
  LOG(0) << "Loading " << file_name;

  renderer_ = renderer;

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
    return false;
  }

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
    LOG(0) << "Failed to read obj file: " << file_name;
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
    LOG(0) << "Failed to read file: " << file_name;
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

  LOG(0) << "- Total vertices: " << vertices.size();

  // Deduplicate vertices.
  std::vector<uint32_t> remap(vertices.size());
  size_t total_vertices = meshopt_generateVertexRemap(
      remap.data(), nullptr, vertices.size(), vertices.data(), vertices.size(),
      sizeof(Vertex));

  std::vector<Vertex> unique_vertices(total_vertices);
  meshopt_remapVertexBuffer(unique_vertices.data(), vertices.data(),
                            vertices.size(), sizeof(Vertex), remap.data());

  LOG(0) << "- Unique vertices: " << unique_vertices.size();

  std::vector<uint32_t> aggregated_indices(total_index_count);

  for (auto& mi : material_indices) {
    int material_id = mi.first;
    auto& indices = mi.second;

    LOG(0) << "- Indices: " << indices.size();

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

  // Optimize vertices
  meshopt_optimizeVertexFetch(unique_vertices.data(), aggregated_indices.data(),
                              aggregated_indices.size(), unique_vertices.data(),
                              unique_vertices.size(), sizeof(Vertex));

  // Create geometry
  geometry_.SetRenderer(renderer);
  geometry_.Create(kPrimitive_Triangles, vertex_description_, kDataType_UInt);
  geometry_.Update(unique_vertices.size(), unique_vertices.data(),
                   aggregated_indices.size(), aggregated_indices.data());

  if (!tex_file_name.empty()) {
    auto image = std::make_unique<Image>();
    if (!image->Load(tex_file_name))
      return false;
    texture_.SetRenderer(renderer);
    texture_.Update(std::move(image));

    desc_set0_ = renderer->CreateDescriptorSet(shader_id, 0,
                                               {{texture_.resource_id()}}, {});
  }

  return true;
}

void Model::Draw(Shader& shader) {
  if (desc_set0_)
    renderer_->ActivateDescriptorSet(desc_set0_);

  for (auto& mesh : meshes_) {
    if (!desc_set0_)
      shader.SetUniform("albedo", mesh.color);
    geometry_.Draw(mesh.num_indices, mesh.index_offset);
  }
}

}  // namespace eng
