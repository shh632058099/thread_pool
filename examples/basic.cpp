#include "thread_pool.h"

#include <iostream>

static void print_message(void *arg)
{
    const char *message = static_cast<const char *>(arg);
    std::cout << message << '\n';
}

int main()
{
    thread_pool_t *pool = nullptr;
    const char *message = "C++ caller using the C thread pool API.";

    if (thread_pool_create(&pool, 2U, 4U) != THREAD_POOL_OK) {
        std::cerr << "Could not create thread pool.\n";
        return 1;
    }

    if (thread_pool_submit(pool, print_message,
                           const_cast<void *>(static_cast<const void *>(message)))
        != THREAD_POOL_OK) {
        std::cerr << "Could not submit task.\n";
        thread_pool_destroy(pool);
        return 1;
    }

    thread_pool_destroy(pool);
    return 0;
}
