#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct total {
    pthread_mutex_t mutex;
    size_t value;
};

struct task_arg {
    struct total *total;
    size_t value;
};

static void add_value(void *opaque)
{
    struct task_arg *arg = (struct task_arg *)opaque;

    (void)pthread_mutex_lock(&arg->total->mutex);
    arg->total->value += arg->value;
    (void)pthread_mutex_unlock(&arg->total->mutex);
}

int main(void)
{
    struct total total;
    struct task_arg tasks[] = {
        { &total, 1U }, { &total, 2U }, { &total, 3U }, { &total, 4U }
    };
    thread_pool_t *pool = NULL;
    thread_pool_status_t status;
    size_t i;

    total.value = 0U;
    if (pthread_mutex_init(&total.mutex, NULL) != 0) {
        (void)fprintf(stderr, "could not initialize mutex\n");
        return EXIT_FAILURE;
    }

    status = thread_pool_create(&pool, 2U, 4U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "create failed: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&total.mutex);
        return EXIT_FAILURE;
    }

    for (i = 0U; i < sizeof(tasks) / sizeof(tasks[0]); ++i) {
        status = thread_pool_submit(pool, add_value, &tasks[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "submit failed: %s\n",
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            (void)pthread_mutex_destroy(&total.mutex);
            return EXIT_FAILURE;
        }
    }

    (void)printf("Tasks still queued: %zu\n", thread_pool_pending(pool));
    thread_pool_destroy(pool);

    (void)printf("Sum after destroy drained all tasks: %zu\n", total.value);
    (void)pthread_mutex_destroy(&total.mutex);
    return total.value == 10U ? EXIT_SUCCESS : EXIT_FAILURE;
}
