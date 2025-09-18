#include "engine/model.h"

#include <iostream>
#include <sstream>

#include "base/log.h"
#include "engine/asset/image.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "engine/renderer/renderer.h"
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
                    const std::string& tex_file_name,
                    uint64_t shader_id) {
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
    LOG(0) << "Failed to read file: " << file_name;
    return false;
  }
  std::istringstream obj_stream(std::istringstream(obj.get()));

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &obj_stream)) {
    LOG(0) << "Failed to read file: " << file_name;
    return false;
  }

  std::vector<Vertex> vertices;
  std::unordered_map<size_t, uint32_t> unique_vertices;

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

        // Deduplicate vertices
        size_t hash = std::hash<float>{}(vert.position[0]) ^
                      std::hash<float>{}(vert.position[1]) ^
                      std::hash<float>{}(vert.position[2]) ^
                      std::hash<float>{}(vert.normal[0]) ^
                      std::hash<float>{}(vert.normal[1]) ^
                      std::hash<float>{}(vert.normal[2]) ^
                      std::hash<float>{}(vert.uv[0]) ^
                      std::hash<float>{}(vert.uv[1]);

        uint32_t new_index;
        auto it = unique_vertices.find(hash);
        if (it != unique_vertices.end()) {
          new_index = it->second;
        } else {
          new_index = (uint32_t)vertices.size();
          vertices.push_back(vert);
          unique_vertices[hash] = new_index;
        }

        material_indices[material_id].push_back(new_index);
      }
      index_offset += fv;
    }
  }

  for (auto& mi : material_indices) {
    // int material_id = mi.first;
    auto& indices = mi.second;

    // Optimize
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(),
                                vertices.size());
    meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(),
                             &vertices[0].position[0], vertices.size(),
                             sizeof(Vertex), 1.05f);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(),
                                vertices.data(), vertices.size(),
                                sizeof(Vertex));

    geometries_.emplace_back(renderer_);
    geometries_.back().Create(kPrimitive_Triangles, vertex_description_,
                              kDataType_UInt);
    geometries_.back().Update(vertices.size(), vertices.data(), indices.size(),
                              indices.data());
  }

  auto image = std::make_unique<Image>();
  if (!image->Load(tex_file_name))
    return false;
  texture_.SetRenderer(renderer);
  texture_.Update(std::move(image));

  desc_set0_ = renderer->CreateDescriptorSet(shader_id, 0,
                                             {{texture_.resource_id()}}, {});

  return true;
}

void Model::Draw() {
  renderer_->ActivateDescriptorSet(desc_set0_);
  for (auto& g : geometries_)
    g.Draw();
}

}  // namespace eng
