A simple, cross-platform 2D game engine with OpenGL and Vulkan renderers.
Supports Linux and Android platforms.
This is a personal hobby project. I've published a little game on
[Google Play](https://play.google.com/store/apps/details?id=com.woom.game)
based on this engine. Full game code and assets are included in this repository.
#### Building the demo
Linux:
```text
cd build/linux
make
```
Android:
```text
cd build/android
./gradlew :app:assembleRelease
```
#### Third-party libraries:
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
[vulkan-sdk](https://vulkan.lunarg.com)
