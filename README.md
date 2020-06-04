# Kaliber

A simple, cross-platform, multi-threaded 2D game engine with OpenGL renderer written in modern C++. Supports Linux and Android platforms.

This is a hobby project I worked on in my spare time. I started from scratch and it took me 4 weeks to get it to work and upstream. My goal was to write modern c++ code that is as simple and clean as possible and to keep performance high.

#### // TODO:

* Sound and music.
* 3D rendering.

#### Build for Linux (gcc or clang):
```
cd build/linux
make
```
#### Build for Android:
```
cd build/android
./gradlew :app:assembleRelease
```
#### Build for Android and install (debug only):
```
cd build/android
./gradlew :app:installDebug
```
