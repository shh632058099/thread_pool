#include "thread_pool.h"

#include <stdio.h>

static void print_task(void *opaque)
{
    const int *task_id = (const int *)opaque;
    (void)printf("Running task %d\n", *task_id);
}

int main(void)
{
    enum { TASK_COUNT = 5 };
    thread_pool_t *pool = NULL;
    int task_ids[TASK_COUNT];
    int i;

    if (thread_pool_create(&pool, 2U, TASK_COUNT) != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool.\n");
        return 1;
    }

    for (i = 0; i < TASK_COUNT; ++i) {
        thread_pool_status_t status;

        task_ids[i] = i + 1;
        status = thread_pool_submit(pool, print_task, &task_ids[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task %d: %s\n",
                          task_ids[i], thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return 1;
        }
    }

    /* Shutdown drains accepted tasks before returning. */
    thread_pool_destroy(pool);
    return 0;
}
