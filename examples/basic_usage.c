#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>

struct task_data {
    pthread_mutex_t mutex;
    int value;
};

static void print_task(void *opaque)
{
    struct task_data *data = (struct task_data *)opaque;

    (void)pthread_mutex_lock(&data->mutex);
    ++data->value;
    (void)printf("Task completed; value = %d\n", data->value);
    (void)pthread_mutex_unlock(&data->mutex);
}

int main(void)
{
    struct task_data data;
    thread_pool_t *pool = NULL;
    thread_pool_status_t status;

    data.value = 0;
    if (pthread_mutex_init(&data.mutex, NULL) != 0) {
        (void)fprintf(stderr, "Could not initialize task mutex.\n");
        return 1;
    }

    status = thread_pool_create(&pool, 2U, 8U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&data.mutex);
        return 1;
    }

    status = thread_pool_submit(pool, print_task, &data);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not submit task: %s\n",
                      thread_pool_status_string(status));
        thread_pool_destroy(pool);
        (void)pthread_mutex_destroy(&data.mutex);
        return 1;
    }

    /* Destruction drains accepted tasks before returning. */
    thread_pool_destroy(pool);
    (void)pthread_mutex_destroy(&data.mutex);
    return 0;
}
