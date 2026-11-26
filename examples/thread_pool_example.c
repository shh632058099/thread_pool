#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>

struct task_argument {
    size_t value;
    size_t result;
};

static void square(void *opaque)
{
    struct task_argument *argument = (struct task_argument *)opaque;
    argument->result = argument->value * argument->value;
}

int main(void)
{
    enum { TASK_COUNT = 10 };
    struct task_argument arguments[TASK_COUNT];
    thread_pool_t *pool = NULL;
    thread_pool_status_t status;
    size_t i;

    status = thread_pool_create(&pool, 2U, TASK_COUNT);
    if (status != THREAD_POOL_OK) {
        (void)fprintf(stderr, "Could not create thread pool: %s\n",
                      thread_pool_status_string(status));
        return EXIT_FAILURE;
    }

    for (i = 0U; i < TASK_COUNT; ++i) {
        arguments[i].value = i;
        arguments[i].result = 0U;
        status = thread_pool_submit(pool, square, &arguments[i]);
        if (status != THREAD_POOL_OK) {
            (void)fprintf(stderr, "Could not submit task %lu: %s\n",
                          (unsigned long)i, thread_pool_status_string(status));
            thread_pool_destroy(pool);
            return EXIT_FAILURE;
        }
    }

    /* Destruction drains accepted work before the argument array goes out of scope. */
    thread_pool_destroy(pool);

    for (i = 0U; i < TASK_COUNT; ++i) {
        (void)printf("%lu squared is %lu\n", (unsigned long)arguments[i].value,
                     (unsigned long)arguments[i].result);
    }
    return EXIT_SUCCESS;
}
