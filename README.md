A simple, cross-platform 2D game engine with OpenGL renderer. Supports Linux and
Android (lolipop+) platforms.
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
GN (linux only for now):
```text
gn gen --args='is_debug=false' out/release
ninja -C out/release
```
#### Third-party libraries:
[glew](https://github.com/nigels-com/glew),
[jsoncpp](https://github.com/open-source-parsers/jsoncpp),
[minimp3](https://github.com/lieff/minimp3),
[oboe](https://github.com/google/oboe),
[r8brain-free-src](https://github.com/avaneev/r8brain-free-src),
[stb](https://github.com/nothings/stb),
[texture-compressor](https://github.com/auygun/kaliber/tree/master/src/third_party/texture_compressor),
[minizip](https://github.com/madler/zlib/tree/master/contrib/minizip)
