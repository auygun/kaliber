#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(native-activity SHARED
    ../../../src/base/file_android.cc
    ../../../src/base/file.cc
    ../../../src/base/image.cc
    ../../../src/base/random.cc
    ../../../src/base/timer.cc
    ../../../src/demo/demo.cc
    ../../../src/engine/engine.cc
    ../../../src/engine/font.cc
    ../../../src/engine/quad.cc
    ../../../src/engine/renderer/geometry.cc
    ../../../src/engine/renderer/renderer_android.cc
    ../../../src/engine/renderer/renderer.cc
    ../../../src/engine/renderer/shader.cc
    ../../../src/engine/renderer/texture.cc
    ../../../src/platform/platform_android.cc
    ../../../src/third_party/android/gl3stub.c
    ../../../src/third_party/android/GLContext.cpp
    ../../../src/third_party/minizip/ioapi.c
    ../../../src/third_party/minizip/unzip.c
)

target_include_directories(native-activity PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ../../../src/base
    ../../../src
    ../../../src/engine/renderer
    ../../../src/platform
    ../../../src/third_party/android
    ../../../src/third_party/minizip
    ../../../src/third_party/texture_compress
    )

# add lib dependencies
target_link_libraries(native-activity
    android
    native_app_glue
    EGL
    GLESv2
    log
    z)
