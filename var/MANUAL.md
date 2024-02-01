## Build: Including all submodules
Clone this repository recursively
```bash
git clone https://github.com/romanpunia/vitex --recursive
```
Generate and build project files while being inside of repository
```bash
# Default
cmake . -DCMAKE_BUILD_TYPE=Release # -DVI_CXX=14
# With vcpkg
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake # -DVI_CXX=14
```
Build project files while being inside of repository
```bash
    cmake --build . --config Release
```

## Build: Including specific submodules
Clone this repository at top level
```bash
git clone https://github.com/romanpunia/vitex
```
Initialize needed submodules while being inside of repository
```bash
# Initialize required submodules:
git submodule update --init ./deps/concurrentqueue

# Initialize optional submodules, for example:
git submodule update --init ./deps/stb
```
Generate and build project files while being inside of repository (don't forget to disable missing submodules)
```bash
# Default
cmake . -DCMAKE_BUILD_TYPE=Release -DVI_ANGELSCRIPT=OFF -DVI_...=OFF # -DVI_CXX=14
# With vcpkg
cmake . -DCMAKE_BUILD_TYPE=Release -DVI_ANGELSCRIPT=OFF -DVI_...=OFF -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake # -DVI_CXX=14
```
Build project files while being inside of repository
```bash
cmake --build . --config Release
```

## Build: As a CMake dependency
Add Vitex toolchain. Add needed dependencies in vcpkg.json near your CMakeLists.txt if you use vcpkg:
```cmake
include(path/to/vitex/deps/toolchain.cmake)
# ...
project(app_name)
```
Add Vitex as subproject.
```cmake
add_subdirectory(/path/to/vitex vitex)
link_directories(/path/to/vitex)
target_include_directories(app_name PRIVATE /path/to/vitex)
target_link_libraries(app_name PRIVATE vitex)
```
Example [CMakeLists.txt](https://github.com/romanpunia/lynx/blob/master/CMakeLists.txt) with Vitex as subproject
