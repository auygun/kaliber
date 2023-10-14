# Kaliber

A simple, cross-platform 2D game engine with OpenGL and Vulkan renderers.
Supports Linux, Windows and Android platforms.
This is a personal hobby project. I've published a little game on
[Google Play](https://play.google.com/store/apps/details?id=com.woom.game)
based on this engine. Full game code and assets are included in this repository.

## Pre-requisites:

GN build system is required for all platforms:\
https://gn.googlesource.com/gn/

## Building from the command-line:

### All platforms except Android:
#### Setup:
Generate build files for Ninja in release and debug modes.
```text
gn gen out/release
gn gen --args='is_debug=true' out/debug
```
#### Building and running:
Build all games in release mode and run "hello_world".
```text
ninja -C out/release
./out/release/hello_world
```
Build "demo" in debug mode and run.
```text
ninja -C out/debug demo
./out/debug/demo
```

### Android:
Build "hello_world" in debug mode for all ABIs and install. GN will be run by
Gradle so no setup is required. Location of gn and ninja executables can be
specified via "gn" and "ninja" properties (-Pgn="path/gn").
```text
cd build/android
./gradlew :app:installHelloWorldAllArchsDebug
```
Build "demo" in debug mode for x86_64 ABI and install. Valid ABI targets are
Arm7, Arm8, X86, X86_64, AllArchs, ArmOnly, X86Only.
```text
./gradlew :app:installDemoX86_64Debug
```

### Generate Visual Studio solution:
```text
gn.exe gen --args="is_debug=true" --ide=vs2022 out\vs
devenv out\vs\all.sln
```

## Hello World example:

Shows a smoothly rotating "Hello World".
```cpp
class HelloWorld final : public eng::Game {
 public:
  bool Initialize() final {
    eng::Engine::Get().SetImageSource(
        "hello_world_image",
        std::bind(&eng::Engine::Print, &eng::Engine::Get(), "Hello World",
                  /*bg_color*/ base::Vector4f(1, 1, 1, 0)));

    hello_world_.Create("hello_world_image").SetVisible(true);
    animator_.Attach(&hello_world_);
    animator_.SetRotation(base::PI2f, /*duration*/ 3,
                          std::bind(base::SmootherStep, std::placeholders::_1));
    animator_.Play(eng::Animator::kRotation, /*loop*/ true);
    return true;
  }

 private:
  eng::ImageQuad hello_world_;
  eng::Animator animator_;
};

GAME_FACTORIES{GAME_CLASS(HelloWorld)};
```

## Third-party libraries:

[glew](https://github.com/nigels-com/glew),
[jsoncpp](https://github.com/open-source-parsers/jsoncpp),
[minimp3](https://github.com/lieff/minimp3),
[oboe](https://github.com/google/oboe),
[stb](https://github.com/nothings/stb),
[texture-compressor](https://github.com/auygun/kaliber/tree/master/src/third_party/texture_compressor),
[minizip](https://github.com/madler/zlib/tree/master/contrib/minizip),
[glslang](https://github.com/KhronosGroup/glslang),
[spirv-reflect](https://github.com/KhronosGroup/SPIRV-Reflect),
[vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator),
[vulkan-sdk](https://vulkan.lunarg.com),
[volk](https://github.com/zeux/volk),
[imgui](https://github.com/ocornut/imgui)
