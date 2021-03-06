.DEFAULT_GOAL := all

# --- Input variables ---
BUILD ?= release
ifeq ($(findstring $(BUILD),debug release),)
  $(error BUILD must be set to debug or release)
endif

# Build all executables by default.
APPS ?= gltest

# If the VERBOSE flag isn't set, then mute superfluous output.
ifeq ($(VERBOSE),)
  HUSH_COMPILE = @echo "Compiling $<";
  HUSH_LINK = @echo "Linking $@";
  HUSH_GENERATE = @echo "Generating $@";
  HUSH_CLEAN = @
endif

# --- Internal variables ---
ARCH := $(shell uname -p)
SRC_ROOT := $(abspath ../../src)
OUTPUT_DIR := $(abspath .)
INTERMEDIATE_DIR := $(OUTPUT_DIR)/obj
BUILD_DIR := $(INTERMEDIATE_DIR)/$(BUILD)

ARFLAGS = r
LDFLAGS = -lX11 -lGL -pthread -lasound -ldl

# Always enable debug information.
CFLAGS += -g

# Flags to generate dependency information.
CFLAGS += -MD -MP -MT $@

# Predefined flags.
ifeq ($(BUILD), debug)
  CFLAGS += -D_DEBUG
  CFLAGS += -D_GLIBCXX_DEBUG
endif

# Enable compiler optimizations for everything except debug.
# Note that a very aggresssive optimization level is used and it may not be
# valid for all standard compliant programs. Reduce this level on individual
# files or modules as needed.
ifneq ($(BUILD), debug)
  CFLAGS += -Ofast
endif

# Flag to turn on extended instruction sets for the compiler.
CFLAGS += -msse2

# Let C++ inherit all C flags.
CXXFLAGS = $(CFLAGS)

# Enable C++17
CXXFLAGS += -std=c++17

# Vulkan config
CFLAGS += -DVK_USE_PLATFORM_XLIB_KHR
CFLAGS += -DVULKAN_NON_CMAKE_BUILD
CFLAGS += -DSYSCONFDIR='"/etc"'
CFLAGS += -DFALLBACK_DATA_DIRS='"/usr/local/share:/usr/share"'
CFLAGS += -DFALLBACK_CONFIG_DIRS='"/etc/xdg"'
CFLAGS += -DHAVE_SECURE_GETENV

# --- Internal functions ---
app_exe = $(OUTPUT_DIR)/$(1)_$(ARCH)_$(BUILD)
objs_from_src = $(patsubst $(SRC_ROOT)/%, $(BUILD_DIR)/%.o, $(basename $(1)))
objs_from_src_in = $(call objs_from_src, $(shell find $(1) -name "*.cc" -o -name "*.cpp" -o -name "*.c"))

# --- gltest application ---
ifneq ($(filter gltest,$(APPS)),)

GLTEST_SRC := \
  $(SRC_ROOT)/base/collusion_test.cc \
  $(SRC_ROOT)/base/log.cc \
  $(SRC_ROOT)/base/random.cc \
  $(SRC_ROOT)/base/sinc_resampler.cc \
  $(SRC_ROOT)/base/task_runner.cc \
  $(SRC_ROOT)/base/timer.cc \
  $(SRC_ROOT)/base/worker.cc \
  $(SRC_ROOT)/demo/credits.cc \
  $(SRC_ROOT)/demo/demo.cc \
  $(SRC_ROOT)/demo/enemy.cc \
  $(SRC_ROOT)/demo/hud.cc \
  $(SRC_ROOT)/demo/menu.cc \
  $(SRC_ROOT)/demo/player.cc \
  $(SRC_ROOT)/demo/sky_quad.cc \
  $(SRC_ROOT)/engine/animatable.cc \
  $(SRC_ROOT)/engine/animator.cc \
  $(SRC_ROOT)/engine/audio/audio_alsa.cc \
  $(SRC_ROOT)/engine/audio/audio_base.cc \
  $(SRC_ROOT)/engine/audio/audio_resource.cc \
  $(SRC_ROOT)/engine/drawable.cc \
  $(SRC_ROOT)/engine/engine.cc \
  $(SRC_ROOT)/engine/font.cc \
  $(SRC_ROOT)/engine/image_quad.cc \
  $(SRC_ROOT)/engine/image.cc \
  $(SRC_ROOT)/engine/mesh.cc \
  $(SRC_ROOT)/engine/persistent_data.cc \
  $(SRC_ROOT)/engine/platform/asset_file_linux.cc \
  $(SRC_ROOT)/engine/platform/asset_file.cc \
  $(SRC_ROOT)/engine/platform/platform_base.cc \
  $(SRC_ROOT)/engine/platform/platform_linux.cc \
  $(SRC_ROOT)/engine/renderer/geometry.cc \
  $(SRC_ROOT)/engine/renderer/opengl/render_command.cc \
  $(SRC_ROOT)/engine/renderer/opengl/renderer_opengl_linux.cc \
  $(SRC_ROOT)/engine/renderer/opengl/renderer_opengl.cc \
  $(SRC_ROOT)/engine/renderer/render_resource.cc \
  $(SRC_ROOT)/engine/renderer/renderer_types.cc \
  $(SRC_ROOT)/engine/renderer/shader.cc \
  $(SRC_ROOT)/engine/renderer/texture.cc \
  $(SRC_ROOT)/engine/renderer/vulkan/renderer_vulkan_linux.cc \
  $(SRC_ROOT)/engine/renderer/vulkan/renderer_vulkan.cc \
  $(SRC_ROOT)/engine/renderer/vulkan/vulkan_context_linux.cc \
  $(SRC_ROOT)/engine/renderer/vulkan/vulkan_context.cc \
  $(SRC_ROOT)/engine/shader_source.cc \
  $(SRC_ROOT)/engine/solid_quad.cc \
  $(SRC_ROOT)/engine/sound_player.cc \
  $(SRC_ROOT)/engine/sound.cc \
  $(SRC_ROOT)/third_party/glew/glew.c \
  $(SRC_ROOT)/third_party/glslang/glslang/CInterface/glslang_c_interface.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/GenericCodeGen/CodeGen.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/GenericCodeGen/Link.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/attribute.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/Constant.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/glslang_tab.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/InfoSink.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/Initialize.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/Intermediate.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/intermOut.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/IntermTraverse.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/iomapper.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/limits.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/linkValidate.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/parseConst.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/ParseContextBase.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/ParseHelper.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/PoolAlloc.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/propagateNoContraction.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/reflection.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/RemoveTree.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/Scan.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/ShaderLang.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/SymbolTable.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/MachineIndependent/Versions.cpp \
  $(SRC_ROOT)/third_party/glslang/glslang/OSDependent/Unix/ossource.cpp \
  $(SRC_ROOT)/third_party/glslang/OGLCompilersDLL/InitializeDll.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/CInterface/spirv_c_interface.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/disassemble.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/doc.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/GlslangToSpv.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/InReadableOrder.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/Logger.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/SpvBuilder.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/SpvPostProcess.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/SPVRemapper.cpp \
  $(SRC_ROOT)/third_party/glslang/SPIRV/SpvTools.cpp \
  $(SRC_ROOT)/third_party/glslang/StandAlone/ResourceLimits.cpp \
  $(SRC_ROOT)/third_party/jsoncpp/jsoncpp.cc \
  $(SRC_ROOT)/third_party/spirv-reflect/spirv_reflect.c \
  $(SRC_ROOT)/third_party/texture_compressor/dxt_encoder_internals.cc \
  $(SRC_ROOT)/third_party/texture_compressor/dxt_encoder.cc \
  $(SRC_ROOT)/third_party/texture_compressor/texture_compressor_etc1.cc \
  $(SRC_ROOT)/third_party/texture_compressor/texture_compressor.cc \
  $(SRC_ROOT)/third_party/vma/vk_mem_alloc.cpp \
  $(SRC_ROOT)/third_party/vulkan/loader/cJSON.c \
  $(SRC_ROOT)/third_party/vulkan/loader/debug_utils.c \
  $(SRC_ROOT)/third_party/vulkan/loader/dev_ext_trampoline.c \
  $(SRC_ROOT)/third_party/vulkan/loader/extension_manual.c \
  $(SRC_ROOT)/third_party/vulkan/loader/loader.c \
  $(SRC_ROOT)/third_party/vulkan/loader/murmurhash.c \
  $(SRC_ROOT)/third_party/vulkan/loader/phys_dev_ext.c \
  $(SRC_ROOT)/third_party/vulkan/loader/trampoline.c \
  $(SRC_ROOT)/third_party/vulkan/loader/unknown_ext_chain.c \
  $(SRC_ROOT)/third_party/vulkan/loader/wsi.c

GLTEST_EXE := $(call app_exe,gltest)
GLTEST_OBJS := $(call objs_from_src, $(GLTEST_SRC))
EXES += $(GLTEST_EXE)
OBJS += $(GLTEST_OBJS)

$(GLTEST_EXE): $(GLTEST_OBJS) $(LIBS)

endif


# --- Build rules ---

# Dependencies.
DEPS = $(OBJS:.o=.d)
-include $(DEPS)

.PHONY: all clean cleanall help

all: $(EXES)

clean:
	@echo "Cleaning..."
	$(HUSH_CLEAN) $(RM) -r $(BUILD_DIR)

cleanall:
	@echo "Cleaning all..."
	$(HUSH_CLEAN) $(RM) -r $(INTERMEDIATE_DIR)

help:
	@echo "BUILD   = Build mode. One of:"
	@echo "          debug (no optimizations)"
	@echo "          release (optimizations, the default)"
	@echo "APPS    = Applications to build. Defaults to all."
	@echo "VERBOSE = Full output from commands if set."

# It's important that libraries are specified last as Ubuntu uses "ld --as-needed" by default.
# Only the static libraries referenced by the object files will be linked into the executable.
# Beware that circular dependencies doesn't work with this flag.
$(EXES):
	@mkdir -p $(@D)
	$(HUSH_LINK) $(CXX) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.a:
	@mkdir -p $(@D)
	$(HUSH_GENERATE) $(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: $(SRC_ROOT)/%.c
	@mkdir -p $(@D)
	$(HUSH_COMPILE) $(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_ROOT)/%.cc
	@mkdir -p $(@D)
	$(HUSH_COMPILE) $(CXX) -c $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_ROOT)/%.cpp
	@mkdir -p $(@D)
	$(HUSH_COMPILE) $(CXX) -c $(CXXFLAGS) -o $@ $<
