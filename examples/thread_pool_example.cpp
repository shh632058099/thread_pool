#include "thread_pool.h"

#include <atomic>
#include <cstdio>

namespace {

void increment(void *opaque)
{
    std::atomic<unsigned int> *completed =
        static_cast<std::atomic<unsigned int> *>(opaque);
    completed->fetch_add(1U, std::memory_order_relaxed);
}

} // namespace

int main()
{
    const unsigned int task_count = 8U;
    std::atomic<unsigned int> completed(0U);
    thread_pool_t *pool = NULL;
    thread_pool_status_t status = thread_pool_create(&pool, 2U, task_count);
    if (status != THREAD_POOL_OK) {
        std::fprintf(stderr, "create failed: %s\n", thread_pool_status_string(status));
        return 1;
    }

    for (unsigned int i = 0U; i < task_count; ++i) {
        status = thread_pool_submit(pool, increment, &completed);
        if (status != THREAD_POOL_OK) {
            std::fprintf(stderr, "submit failed: %s\n", thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return 1;
        }
    }

    std::printf("Queued tasks: %zu\n", thread_pool_pending(pool));
    thread_pool_destroy(pool);

    if (completed.load(std::memory_order_relaxed) != task_count) {
        std::fprintf(stderr, "not all accepted tasks completed\n");
        return 1;
    }
    std::printf("Completed %u tasks; pool drained and workers joined.\n", task_count);
    return 0;
}
