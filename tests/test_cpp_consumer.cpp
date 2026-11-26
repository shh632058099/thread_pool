#include "thread_pool.h"

#include <atomic>

namespace {

void increment(void *opaque)
{
    std::atomic<int> *count = static_cast<std::atomic<int> *>(opaque);
    count->fetch_add(1, std::memory_order_relaxed);
}

} // namespace

int main()
{
    thread_pool_t *pool = nullptr;
    std::atomic<int> count(0);

    if (thread_pool_create(&pool, 1U, 1U) != THREAD_POOL_OK) {
        return 1;
    }
    if (thread_pool_submit(pool, increment, &count) != THREAD_POOL_OK) {
        thread_pool_destroy(pool);
        return 2;
    }

    thread_pool_destroy(pool);
    return count.load(std::memory_order_relaxed) == 1 ? 0 : 3;
}
