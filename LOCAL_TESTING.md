# Local testing

This repository uses CMake with the Ninja generator and CTest. The default CMake configuration enables the `thread_pool_tests` executable and registers the `thread_pool_tests` CTest test.

## Prerequisites

Install the following locally:

- CMake 3.10 or newer
- Ninja
- A C compiler with POSIX threads support

## Configure, build, and test

Run these commands from the repository root:

```sh
cmake -S . -B build -G Ninja -DTHREAD_POOL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The configure step creates a Ninja build tree in `build/`. The build step builds the `thread_pool` library and `thread_pool_tests` executable. The CTest step runs the registered `thread_pool_tests` test and prints its output if it fails.

To rerun the tests after changing source files, use:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```
