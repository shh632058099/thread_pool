# Local Build and Test

Run these commands from the repository root. The project requires CMake 3.10 or newer, Ninja, and a C99 compiler with POSIX threads support.

## Configure

Configure an out-of-source Ninja build with the unit-test target enabled:

```sh
cmake -S . -B build -G Ninja -DTHREAD_POOL_BUILD_TESTS=ON
```

The configuration creates the `thread_pool` library and the `thread_pool_tests` executable. `THREAD_POOL_BUILD_TESTS` is enabled by default, but it is set explicitly above so that CTest is configured for this build.

## Build

```sh
cmake --build build
```

## Test

Run the registered CTest test and show its output when it fails:

```sh
ctest --test-dir build --output-on-failure
```

The test registered by CMake is named `thread_pool_tests`.
