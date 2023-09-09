#ifndef ENGINE_RENDERER_OPENGL_OPENGL_H
#define ENGINE_RENDERER_OPENGL_OPENGL_H

#if defined(__ANDROID__)

// Use the modified Khronos header from ndk-helper. This gives access to
// additional functionality the drivers may expose but which the system headers
// do not.
#include <ndk_helper/gl3stub.h>

#include <GLES2/gl2ext.h>

#elif defined(__linux__)

#include "third_party/glew/glew.h"
#include "third_party/glew/glxew.h"

#elif defined(_WIN32)

#define GLEW_STATIC
#include "third_party/glew/glew.h"
#include "third_party/glew/wglew.h"

#endif

#if defined(__linux__) || defined(_WIN32)

// Define the missing format for the etc1
#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES 0x8D64
#endif
#endif

#endif  // ENGINE_RENDERER_OPENGL_OPENGL_H
