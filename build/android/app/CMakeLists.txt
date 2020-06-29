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

# OBOE Library
set (OBOE_DIR ../../../src/third_party/oboe)
add_subdirectory(${OBOE_DIR} ./oboe-bin)


include(AndroidNdkModules)
android_ndk_import_module_cpufeatures()

# build native_app_glue as a static lib
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS} -D_DEBUG")
else ()
    set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
endif ()
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -D_DEBUG")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror")
endif ()


# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(native-activity SHARED
    ../../../src/base/collusion_test.cc
    ../../../src/base/log.cc
    ../../../src/base/random.cc
    ../../../src/base/task_runner.cc
    ../../../src/base/timer.cc
    ../../../src/base/vecmath.cc
    ../../../src/base/worker.cc
    ../../../src/demo/credits.cc
    ../../../src/demo/demo.cc
    ../../../src/demo/enemy.cc
    ../../../src/demo/hud.cc
    ../../../src/demo/menu.cc
    ../../../src/demo/player.cc
    ../../../src/demo/sky_quad.cc
    ../../../src/engine/animatable.cc
    ../../../src/engine/animator.cc
    ../../../src/engine/audio/audio_base.cc
    ../../../src/engine/audio/audio_oboe.cc
    ../../../src/engine/audio/audio_resource.cc
    ../../../src/engine/engine.cc
    ../../../src/engine/font.cc
    ../../../src/engine/image_quad.cc
    ../../../src/engine/image.cc
    ../../../src/engine/mesh.cc
    ../../../src/engine/platform/asset_file_android.cc
    ../../../src/engine/platform/asset_file.cc
    ../../../src/engine/platform/platform_android.cc
    ../../../src/engine/platform/platform.cc
    ../../../src/engine/renderer/geometry.cc
    ../../../src/engine/renderer/render_command.cc
    ../../../src/engine/renderer/render_resource.cc
    ../../../src/engine/renderer/renderer_android.cc
    ../../../src/engine/renderer/renderer_types.cc
    ../../../src/engine/renderer/renderer.cc
    ../../../src/engine/renderer/shader.cc
    ../../../src/engine/renderer/texture.cc
    ../../../src/engine/shader_source.cc
    ../../../src/engine/solid_quad.cc
    ../../../src/engine/sound_player.cc
    ../../../src/engine/sound.cc
    ../../../src/third_party/android/gestureDetector.cpp
    ../../../src/third_party/android/gl3stub.c
    ../../../src/third_party/android/GLContext.cpp
    ../../../src/third_party/jsoncpp/jsoncpp.cc
    ../../../src/third_party/minizip/ioapi.c
    ../../../src/third_party/minizip/unzip.c
    ../../../src/third_party/r8b/pffft.cpp
    ../../../src/third_party/r8b/r8bbase.cpp
    ../../../src/third_party/texture_compressor/dxt_encoder_internals.cc
    ../../../src/third_party/texture_compressor/dxt_encoder.cc
    ../../../src/third_party/texture_compressor/texture_compressor_etc1.cc
    ../../../src/third_party/texture_compressor/texture_compressor.cc
)

if (ANDROID_ABI STREQUAL armeabi-v7a)
    target_sources(native-activity PRIVATE ../../../src/third_party/texture_compressor/dxt_encoder_neon.cc)
    target_sources(native-activity PRIVATE ../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc)
    set_source_files_properties(../../../src/third_party/r8b/pffft.cpp PROPERTIES COMPILE_FLAGS -mfpu=neon)
    set_source_files_properties(../../../src/third_party/texture_compressor/dxt_encoder_neon.cc PROPERTIES COMPILE_FLAGS -mfpu=neon)
    set_source_files_properties(../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc PROPERTIES COMPILE_FLAGS -mfpu=neon)
endif()

if (ANDROID_ABI STREQUAL arm64-v8a)
    target_sources(native-activity PRIVATE ../../../src/third_party/texture_compressor/dxt_encoder_neon.cc)
    target_sources(native-activity PRIVATE ../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc)
endif()

target_include_directories(native-activity PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    )

# add lib dependencies
target_link_libraries(native-activity
    android
    native_app_glue
    oboe
    cpufeatures
    EGL
    GLESv2
    log
    z)
