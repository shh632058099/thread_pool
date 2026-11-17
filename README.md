# C99 Thread Pool

A small POSIX-pthreads thread pool with a bounded, non-blocking submission queue.
The project uses C99 and CMake.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Development verification

From the repository root, configure with CMake's Ninja generator, build the
project, and run the CTest suite:

```sh
cmake -S . -B build -G Ninja -DTHREAD_POOL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## API

Include `thread_pool.h` and link the `thread_pool` CMake target (which links
`Threads::Threads`).

- `thread_pool_create(&pool, worker_count, queue_capacity)` creates the pool;
  both values must be greater than zero.
- `thread_pool_submit(pool, function, arg)` enqueues a task without waiting for
  queue capacity. It returns `THREAD_POOL_QUEUE_FULL` if pending work fills the
  bounded queue. A task already picked up by a worker is no longer pending.
- `thread_pool_pending(pool)` returns the number of queued (not currently
  executing) tasks.
- `thread_pool_destroy(pool)` rejects new submissions, drains pending tasks,
  joins workers, and frees resources. Do not call concurrently with other API
  calls or from one of the pool's worker callbacks.

The pool does not own task arguments: keep each `arg` alive until its callback
has finished. The pool drains all accepted work at shutdown.

Status results are defined by `thread_pool_status_t`; use
`thread_pool_status_string()` for a readable description.
