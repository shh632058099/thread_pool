#include "thread_pool.h"

#include <stdio.h>

struct task_data {
    unsigned int id;
};

static void print_task(void *opaque)
{
    const struct task_data *task = (const struct task_data *)opaque;
    (void)printf("Running task %u\n", task->id);
}

int main(void)
{
    enum { TASK_COUNT = 8 };
    thread_pool_t *pool = NULL;
    struct task_data tasks[TASK_COUNT];
    thread_pool_status_t status;
    unsigned int i;

    status = thread_pool_create(&pool, 2U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        return 1;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        tasks[i].id = i;
        status = thread_pool_submit(pool, print_task, &tasks[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task %u: %s\n", i,
                          thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return 1;
        }
    }

    /* Destroy drains accepted work, so tasks remains alive through completion. */
    thread_pool_destroy(pool);
    return 0;
}
