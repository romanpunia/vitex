## Build: Including all submodules
Clone this repository recursively
```bash
git clone https://github.com/romanpunia/mavi --recursive
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
git clone https://github.com/romanpunia/mavi
```
Initialize needed submodules while being inside of repository
```bash
# Initialize required submodules
git submodule update --init ./deps/wepoll
git submodule update --init ./deps/concurrentqueue

# Initialize needed for your project submodules, for example
git submodule update --init ./deps/stb
```
Generate and build project files while being inside of repository and also disable missing submodules
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
Add Mavi toolchain. With vcpkg add needed dependencies in vcpkg.json near your CMakeLists.txt:
```cmake
include(path/to/mavi/deps/toolchain.cmake)
# ...
project(app_name)
```
Add Mavi as subproject.
```cmake
add_subdirectory(/path/to/mavi mavi)
link_directories(/path/to/mavi)
target_include_directories(app_name PRIVATE /path/to/mavi)
target_link_libraries(app_name PRIVATE mavi)
```
Example [CMakeLists.txt](https://github.com/romanpunia/lynx/blob/master/CMakeLists.txt) with Mavi as subproject
