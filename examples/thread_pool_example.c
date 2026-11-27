#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct task_data {
    unsigned int value;
    unsigned long *sum;
    pthread_mutex_t *sum_mutex;
};

static void add_square(void *opaque)
{
    struct task_data *data = (struct task_data *)opaque;
    const unsigned long square = (unsigned long)data->value * data->value;

    (void)pthread_mutex_lock(data->sum_mutex);
    *data->sum += square;
    (void)pthread_mutex_unlock(data->sum_mutex);
}

int main(void)
{
    enum { TASK_COUNT = 8 };
    thread_pool_t *pool = NULL;
    struct task_data tasks[TASK_COUNT];
    pthread_mutex_t sum_mutex;
    unsigned long sum = 0UL;
    size_t i;
    thread_pool_status_t status;

    if (pthread_mutex_init(&sum_mutex, NULL) != 0) {
        (void)fprintf(stderr, "Could not initialize sum mutex.\n");
        return EXIT_FAILURE;
    }

    status = thread_pool_create(&pool, 2U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&sum_mutex);
        return EXIT_FAILURE;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        tasks[i].value = (unsigned int)(i + 1U);
        tasks[i].sum = &sum;
        tasks[i].sum_mutex = &sum_mutex;

        status = thread_pool_submit(pool, add_square, &tasks[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task %lu: %s\n",
                          (unsigned long)i,
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            (void)pthread_mutex_destroy(&sum_mutex);
            return EXIT_FAILURE;
        }
    }

    /* Destruction drains accepted tasks before returning. */
    thread_pool_destroy(pool);
    (void)pthread_mutex_destroy(&sum_mutex);

    (void)printf("Sum of squares from 1 to %d: %lu\n", TASK_COUNT, sum);
    return EXIT_SUCCESS;
}
