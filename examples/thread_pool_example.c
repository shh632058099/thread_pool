#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct example_task {
    pthread_mutex_t *mutex;
    size_t *completed;
    size_t number;
};

static void print_task(void *opaque)
{
    struct example_task *task = (struct example_task *)opaque;

    (void)printf("Task %lu ran on a worker thread.\n",
                 (unsigned long)task->number);
    (void)pthread_mutex_lock(task->mutex);
    ++*task->completed;
    (void)pthread_mutex_unlock(task->mutex);
}

int main(void)
{
    enum { TASK_COUNT = 5 };
    thread_pool_t *pool = NULL;
    pthread_mutex_t mutex;
    struct example_task tasks[TASK_COUNT];
    size_t completed = 0U;
    size_t i;
    thread_pool_status_t status;

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        (void)fprintf(stderr, "Could not initialize mutex.\n");
        return EXIT_FAILURE;
    }

    status = thread_pool_create(&pool, 2U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create pool: %s\n",
                      thread_pool_status_string(status));
        (void)pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        tasks[i].mutex = &mutex;
        tasks[i].completed = &completed;
        tasks[i].number = i + 1U;
        status = thread_pool_submit(pool, print_task, &tasks[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task: %s\n",
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            (void)pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }
    }

    /* Destroy drains all accepted tasks before returning. */
    thread_pool_destroy(pool);
    (void)pthread_mutex_destroy(&mutex);
    (void)printf("Completed %lu tasks.\n", (unsigned long)completed);
    return completed == TASK_COUNT ? EXIT_SUCCESS : EXIT_FAILURE;
}
