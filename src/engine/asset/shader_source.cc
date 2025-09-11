#include "engine/asset/shader_source.h"

#include <cstring>

#include "base/log.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"

namespace eng {

bool ShaderSource::Load(const std::string& name) {
  name_ = name;

  vertex_source_size_ =
      LoadInternal(name + "_vertex", vertex_source_, nullptr, 0);
  if (!vertex_source_)
    return false;

  fragment_source_size_ =
      LoadInternal(name + "_fragment", fragment_source_, nullptr, 0);
  if (!fragment_source_)
    return false;

  LOG(0) << "Loaded " << name;

  return true;
}

size_t ShaderSource::LoadInternal(const std::string& name,
                                  std::unique_ptr<char[]>& dst,
                                  const char* inject,
                                  size_t inject_len) {
  size_t size;
  auto source = AssetFile::ReadWholeFile(
      name.c_str(), Engine::Get().GetRootPath().c_str(), &size, true);
  if (!source) {
    LOG(0) << "Failed to read file: " << name;
    return 0;
  }

  // Inject macros.
  size++;  // Include the null-terminator.
  size_t total_size = inject_len + size;
  dst = std::make_unique<char[]>(total_size);
  if (inject_len > 0)
    memcpy(dst.get(), inject, inject_len);
  memcpy(dst.get() + inject_len, source.get(), size);

  return total_size;
}

}  // namespace eng
