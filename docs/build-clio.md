# How to build Clio

Clio is built with [CMake](https://cmake.org/) and uses [Conan](https://conan.io/) for managing dependencies. It is written in C++20 and therefore requires a modern compiler.

## Minimum Requirements

- [Python 3.7](https://www.python.org/downloads/)
- [Conan 1.55](https://conan.io/downloads.html)
- [CMake 3.16](https://cmake.org/download/)
- [**Optional**] [GCovr](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html): needed for code coverage generation
- [**Optional**] [CCache](https://ccache.dev/): speeds up compilation if you are going to compile Clio often

| Compiler    | Version |
|-------------|---------|
| GCC         | 11      |
| Clang       | 14      |
| Apple Clang | 14.0.3  |

### Conan Configuration

Clio does not require anything but default settings in your (`~/.conan/profiles/default`) Conan profile. It's best to have no extra flags specified.

> Mac example:

```
[settings]
os=Macos
os_build=Macos
arch=armv8
arch_build=armv8
compiler=apple-clang
compiler.version=14
compiler.libcxx=libc++
build_type=Release
compiler.cppstd=20
```

> Linux example:

```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=gcc
compiler.version=11
compiler.libcxx=libstdc++11
build_type=Release
compiler.cppstd=20
```

#### Artifactory

Make sure artifactory is setup with Conan.

```sh
conan remote add --insert 0 conan-non-prod http://18.143.149.228:8081/artifactory/api/conan/conan-non-prod
```

Now you should be able to download the prebuilt `xrpl` package on some platforms.

> [!NOTE]
> You may need to edit the `~/.conan/remotes.json` file to ensure that this newly added artifactory is listed last. Otherwise, you could see compilation errors when building the project with gcc version 13 (or newer).

Remove old packages you may have cached.

```sh
conan remove -f xrpl
```

## Building Clio

Navigate to Clio's root directory and run:

```sh
mkdir build && cd build
conan install .. --output-folder . --build missing --settings build_type=Release -o tests=True -o lint=False
cmake -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 8 # or without the number if you feel extra adventurous
```

> [!TIP]
> You can omit the `-o tests=True` if you don't want to build `clio_tests`.

If successful, `conan install` will find the required packages and `cmake` will do the rest. You should see `clio_server` and `clio_tests` in the `build` directory (the current directory).

> [!TIP]
> To generate a Code Coverage report, include `-o coverage=True` in the `conan install` command above, along with `-o tests=True` to enable tests. After running the `cmake` commands, execute `make clio_tests-ccov`. The coverage report will be found at `clio_tests-llvm-cov/index.html`.

## Building Clio with Docker

It is also possible to build Clio using [Docker](https://www.docker.com/) if you don't want to install all the dependencies on your machine.

```sh
docker run -it rippleci/clio_ci:latest
git clone https://github.com/XRPLF/clio
mkdir build && cd build
conan install .. --output-folder . --build missing --settings build_type=Release -o tests=True -o lint=False
cmake -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 8 # or without the number if you feel extra adventurous
```

## Developing against `rippled` in standalone mode

If you wish to develop against a `rippled` instance running in standalone mode there are a few quirks of both Clio and `rippled` that you need to keep in mind. You must:

1. Advance the `rippled` ledger to at least ledger 256.
2. Wait 10 minutes before first starting Clio against this standalone node.