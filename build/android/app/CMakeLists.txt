cmake_minimum_required(VERSION 3.22.1)

# common_config
add_library(common_config INTERFACE)

target_compile_options(common_config INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>
    #$<$<NOT:$<CONFIG:DEBUG>>:-Ofast>
    -Wall
    -Werror
    -Wno-unused-but-set-variable
    -Wno-deprecated-enum-enum-conversion
    -Wno-unsequenced
    -Wno-nullability-completeness
)

target_compile_definitions(common_config INTERFACE
    VK_USE_PLATFORM_ANDROID_KHR
    VMA_STATIC_VULKAN_FUNCTIONS=1
    $<$<CONFIG:DEBUG>:_DEBUG>
)

target_include_directories(common_config INTERFACE
    ../../../src
    ../../../src/third_party/glslang
    ../../../src/third_party/vulkan/include
)

# oboe
set (OBOE_DIR ../../../src/third_party/oboe)
add_subdirectory(${OBOE_DIR} ./oboe-bin)

# cpufeatures
add_library(cpufeatures STATIC ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
target_include_directories(cpufeatures PUBLIC ${ANDROID_NDK}/sources/android/cpufeatures)

# native_app_glue
add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
set_property(TARGET native_app_glue PROPERTY POSITION_INDEPENDENT_CODE ON)

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# kaliber
project(kaliber)

add_library(kaliber SHARED
    ../../../src/base/collusion_test.cc
    ../../../src/base/log.cc
    ../../../src/base/mem.cc
    ../../../src/base/task_runner.cc
    ../../../src/base/thread_pool.cc
    ../../../src/demo/credits.cc
    ../../../src/demo/demo.cc
    ../../../src/demo/enemy.cc
    ../../../src/demo/hud.cc
    ../../../src/demo/menu.cc
    ../../../src/demo/player.cc
    ../../../src/demo/sky_quad.cc
    ../../../src/engine/animatable.cc
    ../../../src/engine/animator.cc
    ../../../src/engine/asset/font.cc
    ../../../src/engine/asset/image.cc
    ../../../src/engine/asset/mesh.cc
    ../../../src/engine/asset/shader_source.cc
    ../../../src/engine/asset/sound.cc
    ../../../src/engine/audio/audio_bus.cc
    ../../../src/engine/audio/audio_mixer.cc
    ../../../src/engine/audio/audio_sink_oboe.cc
    ../../../src/engine/audio/mixer_input.cc
    ../../../src/engine/audio/sinc_resampler.cc
    ../../../src/engine/drawable.cc
    ../../../src/engine/engine.cc
    ../../../src/engine/image_quad.cc
    ../../../src/engine/persistent_data.cc
    ../../../src/engine/platform/asset_file_android.cc
    ../../../src/engine/platform/asset_file.cc
    ../../../src/engine/platform/platform_android.cc
    ../../../src/engine/renderer/geometry.cc
    ../../../src/engine/renderer/opengl/renderer_opengl_android.cc
    ../../../src/engine/renderer/opengl/renderer_opengl.cc
    ../../../src/engine/renderer/renderer_types.cc
    ../../../src/engine/renderer/shader.cc
    ../../../src/engine/renderer/texture.cc
    ../../../src/engine/renderer/vulkan/renderer_vulkan_android.cc
    ../../../src/engine/renderer/vulkan/renderer_vulkan.cc
    ../../../src/engine/renderer/vulkan/vulkan_context_android.cc
    ../../../src/engine/renderer/vulkan/vulkan_context.cc
    ../../../src/engine/solid_quad.cc
    ../../../src/engine/sound_player.cc
    ../../../src/third_party/android/gl3stub.c
    ../../../src/third_party/android/GLContext.cpp
    ../../../src/third_party/glslang/glslang/GenericCodeGen/CodeGen.cpp
    ../../../src/third_party/glslang/glslang/GenericCodeGen/Link.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/attribute.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/Constant.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/glslang_tab.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/InfoSink.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/Initialize.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/Intermediate.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/intermOut.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/IntermTraverse.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/iomapper.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/limits.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/linkValidate.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/parseConst.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/ParseContextBase.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/ParseHelper.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/PoolAlloc.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/propagateNoContraction.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/reflection.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/RemoveTree.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/Scan.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/ShaderLang.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/SymbolTable.cpp
    ../../../src/third_party/glslang/glslang/MachineIndependent/Versions.cpp
    ../../../src/third_party/glslang/glslang/OSDependent/Unix/ossource.cpp
    ../../../src/third_party/glslang/OGLCompilersDLL/InitializeDll.cpp
    ../../../src/third_party/glslang/SPIRV/disassemble.cpp
    ../../../src/third_party/glslang/SPIRV/doc.cpp
    ../../../src/third_party/glslang/SPIRV/GlslangToSpv.cpp
    ../../../src/third_party/glslang/SPIRV/InReadableOrder.cpp
    ../../../src/third_party/glslang/SPIRV/Logger.cpp
    ../../../src/third_party/glslang/SPIRV/SpvBuilder.cpp
    ../../../src/third_party/glslang/SPIRV/SpvPostProcess.cpp
    ../../../src/third_party/glslang/SPIRV/SPVRemapper.cpp
    ../../../src/third_party/glslang/SPIRV/SpvTools.cpp
    ../../../src/third_party/jsoncpp/jsoncpp.cpp
    ../../../src/third_party/minizip/ioapi.c
    ../../../src/third_party/minizip/unzip.c
    ../../../src/third_party/spirv-reflect/spirv_reflect.c
    ../../../src/third_party/texture_compressor/dxt_encoder_internals.cc
    ../../../src/third_party/texture_compressor/dxt_encoder.cc
    ../../../src/third_party/texture_compressor/texture_compressor_etc1.cc
    ../../../src/third_party/texture_compressor/texture_compressor.cc
    ../../../src/third_party/vma/vk_mem_alloc.cpp
    ../../../src/third_party/volk/volk.c
)

if (ANDROID_ABI STREQUAL armeabi-v7a)
    target_sources(kaliber PRIVATE ../../../src/third_party/texture_compressor/dxt_encoder_neon.cc)
    target_sources(kaliber PRIVATE ../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc)
    set_source_files_properties(../../../src/third_party/texture_compressor/dxt_encoder_neon.cc PROPERTIES COMPILE_FLAGS -mfpu=neon)
    set_source_files_properties(../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc PROPERTIES COMPILE_FLAGS -mfpu=neon)
endif()

if (ANDROID_ABI STREQUAL arm64-v8a)
    target_sources(kaliber PRIVATE ../../../src/third_party/texture_compressor/dxt_encoder_neon.cc)
    target_sources(kaliber PRIVATE ../../../src/third_party/texture_compressor/texture_compressor_etc1_neon.cc)
endif()

target_link_libraries(kaliber PRIVATE
    common_config
    android
    native_app_glue
    oboe
    cpufeatures
    EGL
    GLESv2
    log
    z
)
