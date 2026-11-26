#include "thread_pool.h"

static void mark_completed(void *opaque)
{
    *static_cast<bool *>(opaque) = true;
}

int main()
{
    thread_pool_t *pool = nullptr;
    bool completed = false;

    if (thread_pool_create(&pool, 1U, 1U) != THREAD_POOL_OK) {
        return 1;
    }
    if (thread_pool_submit(pool, mark_completed, &completed) != THREAD_POOL_OK) {
        thread_pool_destroy(pool);
        return 1;
    }
    thread_pool_destroy(pool);
    return completed ? 0 : 1;
}
