// Define strict types for tinygltf to avoid image writing/loading dependencies
// and filesystem dependencies (we will provide our own FS callbacks).
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_FS
#define TINYGLTF_IMPLEMENTATION
#include "third_party/tinygltf/tiny_gltf.h"

// Stub implementations for tinygltf to satisfy the linker.
// We return true to indicate "success" so tinygltf doesn't abort loading, but
// we intentionally do not load any pixel data. We use image->uri to load the
// texture via the engine's asset system.
namespace tinygltf {

bool LoadImageData(Image* image,
                   const int image_idx,
                   std::string* err,
                   std::string* warn,
                   int req_width,
                   int req_height,
                   const unsigned char* bytes,
                   int size,
                   void* user_data) {
  (void)image;
  (void)image_idx;
  (void)err;
  (void)warn;
  (void)req_width;
  (void)req_height;
  (void)bytes;
  (void)size;
  (void)user_data;
  return true;
}

bool WriteImageData(const std::string* basepath,
                    const std::string* filename,
                    const Image* image,
                    bool embedImages,
                    const FsCallbacks* fs_cb,
                    const URICallbacks* uri_cb,
                    std::string* out_uri,
                    void* user_data) {
  (void)basepath;
  (void)filename;
  (void)image;
  (void)embedImages;
  (void)fs_cb;
  (void)uri_cb;
  (void)out_uri;
  (void)user_data;
  return true;
}

}  // namespace tinygltf
