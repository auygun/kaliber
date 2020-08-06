[![Build Status](https://travis-ci.org/auygun/kaliber.svg?branch=master)](https://travis-ci.org/auygun/kaliber)

A simple, cross-platform 2D game engine with OpenGL renderer. Supports Linux and
Android (lolipop+) platforms, real-time texture compression (ATC, DXT and ETC1) and streaming audio.
Supported asset formats: PNG and JPEG for images, MP3 for sounds, TTF for text rendering.
#### Building the demo
Linux (gcc or clang):
```text
cd build/linux
make
```
Android:
```text
cd build/android
./gradlew :app:assembleRelease
```
GN (linux only for now):
```text
gn gen --args='is_debug=false' out/release
ninja -C out/release
```
#### Todo
IOS platform support and Vulkan renderer.
#### Third-party libraries:
[nigels-com/glew](https://github.com/nigels-com/glew),
[open-source-parsers/jsoncpp](https://github.com/open-source-parsers/jsoncpp),
[lieff/minimp3](https://github.com/lieff/minimp3),
[google/oboe](https://github.com/google/oboe),
[avaneev/r8brain-free-src](https://github.com/avaneev/r8brain-free-src),
[nothings/stb](https://github.com/nothings/stb),
[texture-compressor](https://github.com/chromium/chromium),
[minizip](https://github.com/madler/zlib/tree/master/contrib/minizip)
