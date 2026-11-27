#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>

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
    enum { TASK_COUNT = 10 };
    struct counter counter = { PTHREAD_MUTEX_INITIALIZER, 0U };
    thread_pool_t *pool = NULL;
    thread_pool_status_t status;
    size_t i;

    status = thread_pool_create(&pool, 4U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&counter.mutex);
        return 1;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        status = thread_pool_submit(pool, increment, &counter);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task: %s\n",
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            (void)pthread_mutex_destroy(&counter.mutex);
            return 1;
        }
    }

    /* Destroy drains accepted tasks before returning. */
    thread_pool_destroy(pool);

    (void)printf("Completed %zu tasks.\n", counter.value);
    (void)pthread_mutex_destroy(&counter.mutex);
    return counter.value == TASK_COUNT ? 0 : 1;
}
