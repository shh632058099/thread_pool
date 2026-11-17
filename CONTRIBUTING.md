# 本地验证

本项目使用 CMake 构建，并通过 Ninja 生成和执行测试目标。仓库当前的 CMake 配置定义了 `thread_pool` 库、`thread_pool_tests` 测试可执行文件，以及 CTest 测试 `thread_pool_tests`；测试默认开启。

在仓库根目录执行：

```sh
cmake -S . -B build -G Ninja -DTHREAD_POOL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

如果 `build` 目录之前使用了其他 CMake 生成器配置，请先删除该目录，再重新执行上述命令。验证通过的标准是构建成功，并且 CTest 报告 `thread_pool_tests` 通过。
