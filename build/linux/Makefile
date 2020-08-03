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
LDFLAGS = -lX11 -lGL -pthread -lasound

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
  $(SRC_ROOT)/base/task_runner.cc \
  $(SRC_ROOT)/base/timer.cc \
  $(SRC_ROOT)/base/vecmath.cc \
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
  $(SRC_ROOT)/engine/platform/asset_file_linux.cc \
  $(SRC_ROOT)/engine/platform/asset_file.cc \
  $(SRC_ROOT)/engine/platform/platform_base.cc \
  $(SRC_ROOT)/engine/platform/platform_linux.cc \
  $(SRC_ROOT)/engine/renderer/geometry.cc \
  $(SRC_ROOT)/engine/renderer/render_command.cc \
  $(SRC_ROOT)/engine/renderer/render_resource.cc \
  $(SRC_ROOT)/engine/renderer/renderer_linux.cc \
  $(SRC_ROOT)/engine/renderer/renderer_types.cc \
  $(SRC_ROOT)/engine/renderer/renderer.cc \
  $(SRC_ROOT)/engine/renderer/shader.cc \
  $(SRC_ROOT)/engine/renderer/texture.cc \
  $(SRC_ROOT)/engine/shader_source.cc \
  $(SRC_ROOT)/engine/solid_quad.cc \
  $(SRC_ROOT)/engine/sound_player.cc \
  $(SRC_ROOT)/engine/sound.cc \
  $(SRC_ROOT)/third_party/glew/glew.c \
  $(SRC_ROOT)/third_party/jsoncpp/jsoncpp.cc \
  $(SRC_ROOT)/third_party/r8b/pffft.cpp \
  $(SRC_ROOT)/third_party/r8b/r8bbase.cpp \
  $(SRC_ROOT)/third_party/texture_compressor/dxt_encoder_internals.cc \
  $(SRC_ROOT)/third_party/texture_compressor/dxt_encoder.cc \
  $(SRC_ROOT)/third_party/texture_compressor/texture_compressor_etc1.cc \
  $(SRC_ROOT)/third_party/texture_compressor/texture_compressor.cc

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
