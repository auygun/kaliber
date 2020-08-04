A simple, cross-platform 2D game engine with OpenGL renderer. Supports Linux and
Android platforms.

Build for Linux (gcc or clang):

cd build/linux  
make

Build for Android:

cd build/android  
./gradlew :app:assembleRelease

Build using GN (linux only for now):

gn gen --args='is_debug=false' out/release  
ninja -C out/release
