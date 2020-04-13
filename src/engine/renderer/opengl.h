#ifndef OPENGL_H
#define OPENGL_H

#if defined(__ANDROID__)
#include "../../third_party/android/gl3stub.h"

#elif defined(__linux__)
# include "../../third_party/glew/glew.h"

/* GL_OES_compressed_ETC1_RGB8_texture */
#ifndef GL_OES_compressed_ETC1_RGB8_texture
  #define GL_ETC1_RGB8_OES                             0x8D64
#endif

#endif

#endif // OPENGL_H
