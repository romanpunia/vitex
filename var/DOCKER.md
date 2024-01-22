
## Build: As a Docker image
There are example docker files that can be used to build this project with all optional dependencies installed. Resulting files will be installed to following paths:
```sh
/usr/local/include
/usr/local/lib
```

To customize your build you may use following docker build-arg arguments:
```sh
$CONFIGURE # CMake configuration arguments
$COMPILE   # Compiler configuration arguments
```

### Image: GCC based
```sh
docker build -f var/gcc.Dockerfile -t mavi:staging .
```