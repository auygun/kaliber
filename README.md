## Kaliber

A simple, cross-platform 2D game engine with OpenGL renderer. Supports Linux and
Android platforms.

### Features
* written in modern C++
* Cross-platform and multi-threaded design
* Linux and Android (lolipop+) platforms
* OpenGL 3.2+, OpenGL ES 3.0 rendering
* ALSA, OBOE audio
* PNG, JPEG, MP3 and TTF asset formats
* ATC, DXT and ETC1 texture compression formats
* Streaming sound

### Building
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

### TODO
* Support MacOS, IOS and Windows platforms.
* Vulkan renderer.
* String tables for multi-language support.
* UI layer

### Third-party libraries:
- [nigels-com/glew](https://github.com/nigels-com/glew)
- [open-source-parsers/jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [lieff/minimp3](https://github.com/lieff/minimp3)
- [google/oboe](https://github.com/google/oboe)
- [avaneev/r8brain-free-src](https://github.com/avaneev/r8brain-free-src)
- [nothings/stb](https://github.com/nothings/stb)
- [texture-compressor](https://www.chromium.org/)
- [minizip](https://github.com/madler/zlib/tree/master/contrib/minizip)
