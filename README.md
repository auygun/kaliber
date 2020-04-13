A simple, cross-platform, multi-threaded 2D game engine with OpenGL renderer written in modern
C++. Supports Linux and Android platforms.

Build for Linux (gcc or clang):

cd build/linux  
make

Build for Android:

cd build/android  
./gradlew :app:assembleRelease

Build for Android and install (debug):

cd build/android  
./gradlew :app:installDebug
