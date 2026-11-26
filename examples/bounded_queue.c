#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>

struct task_gate {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int started;
    int release;
};

struct task_data {
    struct task_gate *gate;
    unsigned int *completed;
};

static void wait_task(void *opaque)
{
    struct task_data *data = (struct task_data *)opaque;

    (void)pthread_mutex_lock(&data->gate->mutex);
    data->gate->started = 1;
    (void)pthread_cond_broadcast(&data->gate->condition);
    while (!data->gate->release) {
        (void)pthread_cond_wait(&data->gate->condition, &data->gate->mutex);
    }
    (void)pthread_mutex_unlock(&data->gate->mutex);
    ++*data->completed;
}

static void release_task(struct task_gate *gate)
{
    (void)pthread_mutex_lock(&gate->mutex);
    gate->release = 1;
    (void)pthread_cond_broadcast(&gate->condition);
    (void)pthread_mutex_unlock(&gate->mutex);
}

int main(void)
{
    thread_pool_t *pool = NULL;
    struct task_gate gate = { PTHREAD_MUTEX_INITIALIZER,
                              PTHREAD_COND_INITIALIZER, 0, 0 };
    unsigned int completed = 0U;
    struct task_data first_task = { &gate, &completed };
    struct task_data second_task = { &gate, &completed };
    thread_pool_status_t status;
    int result = 1;

    status = thread_pool_create(&pool, 1U, 1U);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        goto cleanup;
    }

    status = thread_pool_submit(pool, wait_task, &first_task);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not submit first task: %s\n",
                      thread_pool_status_string(status));
        goto cleanup;
    }

    (void)pthread_mutex_lock(&gate.mutex);
    while (!gate.started) {
        (void)pthread_cond_wait(&gate.condition, &gate.mutex);
    }
    (void)pthread_mutex_unlock(&gate.mutex);

    status = thread_pool_submit(pool, wait_task, &second_task);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not queue second task: %s\n",
                      thread_pool_status_string(status));
        goto cleanup;
    }

    status = thread_pool_submit(pool, wait_task, &second_task);
    if (status != THREAD_POOL_QUEUE_FULL) {
        (void)fprintf(stderr, "Expected full queue, got: %s\n",
                      thread_pool_status_string(status));
        goto cleanup;
    }
    (void)puts("Queue is full; submission was rejected without blocking.");
    result = 0;

cleanup:
    release_task(&gate);
    if (pool != NULL) {
        /* The pool drains accepted tasks before returning. */
        thread_pool_destroy(pool);
    }
    (void)pthread_cond_destroy(&gate.condition);
    (void)pthread_mutex_destroy(&gate.mutex);
    if (result == 0) {
        (void)printf("Completed %u accepted tasks.\n", completed);
        if (completed != 2U) {
            result = 1;
        }
    }
    return result;
}
