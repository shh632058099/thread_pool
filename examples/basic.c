#include "thread_pool.h"

#include <stdio.h>

static void print_message(void *arg)
{
    const char *message = (const char *)arg;
    (void)printf("%s\n", message);
}

int main(void)
{
    thread_pool_t *pool = NULL;
    const char *message = "Task completed by a thread pool worker.";
    thread_pool_status_t status;

    status = thread_pool_create(&pool, 2U, 4U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        return 1;
    }

    status = thread_pool_submit(pool, print_message, (void *)message);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not submit task: %s\n",
                      thread_pool_status_string(status));
        thread_pool_destroy(pool);
        return 1;
    }

    /* Destruction drains accepted tasks before returning. */
    thread_pool_destroy(pool);
    return 0;
}
