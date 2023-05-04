# lastest-stop

## Building

First, extract the SDL2 source code into `external/SDL2`, and the SDL_ttf source code into `external/SDL2_ttf`.

Spelunk around inside the SDL2_ttf CMakeLists.txt to configure the project as a static library (not shared). It should be a one-liner option something like:

    option(BUILD_SHARED_LIBS "Build the library as a shared library" OFF)

Then use CMake from the root directory as follows.

On Windows (**untested, probably broken**):

```
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

On Mac/Linux:

```
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S . -B build
cmake --build build --config Release
```