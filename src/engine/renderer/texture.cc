#include "texture.h"
#include "../../base/image.h"
#include "../../base/log.h"

namespace engine {

Texture::~Texture() {
  Destroy();
}

bool Texture::Create(const Image& image) {
  Destroy();

  glGenTextures(1, &id_);

  if (!Update(image))
    return false;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return true;
}

void Texture::Destroy() {
  if (id_)
    glDeleteTextures(1, &id_);
}

void Texture::Activate() {
  glBindTexture(GL_TEXTURE_2D, id_);
}

bool Texture::Update(const Image& image) {
  Activate();

  if (image.IsCompressed()) {
    GLenum format;
    switch (image.GetFormat()) {
      case Image::kDXT1:
        format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        break;
      case Image::kDXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
      case Image::kETC1:
        format = GL_ETC1_RGB8_OES;
        break;
#if defined(__ANDROID__)
      case Image::kATC:
        format = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        break;
#endif
      default:
        LOG("Unknown image format in texture upload\n");
        return false;
    }

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, image.GetWidth(),
                           image.GetHeight(), 0, image.GetSize(),
                           image.GetBuffer());

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      LOG("GL ERROR after glCompressedTexImage2D: %d", (int)err);
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.GetWidth(), image.GetHeight(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image.GetBuffer());
  }

  return true;
}

}  // namespace engine
