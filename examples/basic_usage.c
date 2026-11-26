#include "thread_pool.h"

#include <stdio.h>

static void print_message(void *opaque)
{
    const char *message = (const char *)opaque;
    (void)puts(message);
}

int main(void)
{
    thread_pool_t *pool = NULL;
    const char message[] = "Hello from a thread pool worker.";
    thread_pool_status_t status;

    status = thread_pool_create(&pool, 1U, 1U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "thread_pool_create: %s\n",
                      thread_pool_status_string(status));
        return 1;
    }

    status = thread_pool_submit(pool, print_message, (void *)message);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "thread_pool_submit: %s\n",
                      thread_pool_status_string(status));
        thread_pool_destroy(pool);
        return 1;
    }

    thread_pool_destroy(pool);
    return 0;
}
