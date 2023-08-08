# Kaliber

A simple, cross-platform 2D game engine with OpenGL and Vulkan renderers.
Supports Linux, Windows and Android platforms.
This is a personal hobby project. I've published a little game on
[Google Play](https://play.google.com/store/apps/details?id=com.woom.game)
based on this engine. Full game code and assets are included in this repository.

## Pre-requisites:

**GN build system** is required for all platforms except Android:\
https://gn.googlesource.com/gn/

**Build Tools** is required to build for Windows. if you prefer, you can install
**Visual Studio** which includes the **Build Tools**.

Linux is the supported host platform to build for Android. **Gradle**,
**Android SDK** and **NDK** are required. If you prefer, you can install
**Android Studio** which includes all the requirements.

## Building from the command-line:

### All platforms except Android:
Setup:
```text
gn gen out/release
gn gen --args="is_debug=true" out/debug
```
Building and running:
```text
ninja -C out/debug
./out/debug/hello_world
./out/debug/demo
```
Building and debugging from VS Code:
* Select "Debug demo - [platform]" from the "Run and Debug" drop down.
* Press F5.
### Android:
```text
cd build/android
./gradlew :app:installDebug
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
[volk](https://github.com/zeux/volk)

## Hello World example:

Shows a smoothly rotating "Hello Wolrd!".
```cpp
class HelloWorld final : public eng::Game {
 public:
  ~HelloWorld() final = default;

  bool Initialize() final {
    eng::Engine::Get().SetImageSource(
        "hello_world_image",
        std::bind(&eng::Engine::Print, &eng::Engine::Get(), "Hello World!",
                  base::Vector4f(1, 1, 1, 0)));

    hello_world_.Create("hello_world_image").SetVisible(true);
    animator_.Attach(&hello_world_);
    animator_.SetRotation(base::PI2f, 3,
                          std::bind(base::SmootherStep, std::placeholders::_1));
    animator_.Play(eng::Animator::kRotation, true);
    return true;
  }

 private:
  eng::ImageQuad hello_world_;
  eng::Animator animator_;
};

GAME_FACTORIES{GAME_CLASS(HelloWorld)};
```
