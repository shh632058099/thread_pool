#include "thread_pool.h"

#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace {

struct Counter {
    std::mutex mutex;
    int value;
};

void increment(void *opaque)
{
    Counter *counter = static_cast<Counter *>(opaque);
    std::lock_guard<std::mutex> lock(counter->mutex);
    ++counter->value;
}

} // namespace

int main()
{
    const int task_count = 1000;
    Counter counter;
    thread_pool_t *pool = NULL;
    thread_pool_status_t status;

    counter.value = 0;
    status = thread_pool_create(&pool, 4U, static_cast<size_t>(task_count));
    if (status != THREAD_POOL_OK) {
        std::fprintf(stderr, "Could not create thread pool: %s\n",
                     thread_pool_status_string(status));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < task_count; ++i) {
        status = thread_pool_submit(pool, increment, &counter);
        if (status != THREAD_POOL_OK) {
            std::fprintf(stderr, "Could not submit task %d: %s\n", i,
                         thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return EXIT_FAILURE;
        }
    }

    /* Destruction drains tasks, so the callback argument remains alive. */
    thread_pool_destroy(pool);

    if (counter.value != task_count) {
        std::fprintf(stderr, "Expected %d increments, got %d\n",
                     task_count, counter.value);
        return EXIT_FAILURE;
    }

    std::printf("Completed %d tasks from C++11.\n", counter.value);
    return EXIT_SUCCESS;
}
