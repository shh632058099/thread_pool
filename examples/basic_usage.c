#include "thread_pool.h"

#include <stdio.h>

static void print_value(void *opaque)
{
    const int *value = (const int *)opaque;
    (void)printf("task value: %d\n", *value);
}

int main(void)
{
    thread_pool_t *pool = NULL;
    int values[] = { 1, 2, 3, 4 };
    size_t i;
    thread_pool_status_t status;

    status = thread_pool_create(&pool, 2U, 4U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        return 1;
    }

    for (i = 0U; i < sizeof(values) / sizeof(values[0]); ++i) {
        status = thread_pool_submit(pool, print_value, &values[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "could not submit task: %s\n",
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return 1;
        }
    }

    /* Destroy drains accepted tasks before returning, so values stay alive. */
    thread_pool_destroy(pool);
    return 0;
}
