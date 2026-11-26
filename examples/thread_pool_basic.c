#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct counter {
    pthread_mutex_t mutex;
    size_t value;
};

static void increment(void *opaque)
{
    struct counter *counter = (struct counter *)opaque;

    (void)pthread_mutex_lock(&counter->mutex);
    ++counter->value;
    (void)pthread_mutex_unlock(&counter->mutex);
}

int main(void)
{
    enum { TASK_COUNT = 1000 };
    struct counter counter = { PTHREAD_MUTEX_INITIALIZER, 0U };
    thread_pool_t *pool = NULL;
    size_t i;
    thread_pool_status_t status;

    status = thread_pool_create(&pool, 4U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&counter.mutex);
        return EXIT_FAILURE;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        status = thread_pool_submit(pool, increment, &counter);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task %lu: %s\n",
                          (unsigned long)i, thread_pool_status_string(status));
            thread_pool_destroy(pool);
            (void)pthread_mutex_destroy(&counter.mutex);
            return EXIT_FAILURE;
        }
    }

    /* Destroy drains accepted work before the stack-allocated counter expires. */
    thread_pool_destroy(pool);
    (void)pthread_mutex_destroy(&counter.mutex);

    if (counter.value != TASK_COUNT) {
        (void)fprintf(stderr, "Expected %u increments, got %lu\n",
                      (unsigned int)TASK_COUNT, (unsigned long)counter.value);
        return EXIT_FAILURE;
    }

    (void)printf("Completed %lu tasks.\n", (unsigned long)counter.value);
    return EXIT_SUCCESS;
}
