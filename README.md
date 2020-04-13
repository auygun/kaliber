A simple, cross-platform 2D game engine with OpenGL renderer written in modern
C++. Supports Linux and Android platforms. Compiles both with gcc and clang.

Build for Linux:

cd build/linux
make

Build for Android:

cd build/android
./gradlew :app:assembleRelease

Build for Android and install (debug):

cd build/android
./gradlew :app:installDebug
