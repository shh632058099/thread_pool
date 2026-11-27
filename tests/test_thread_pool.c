#define _POSIX_C_SOURCE 199309L
#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                      __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

struct counter {
    pthread_mutex_t mutex;
    size_t value;
};

struct producer {
    thread_pool_t *pool;
    struct counter *counter;
    pthread_mutex_t *start_mutex;
    pthread_cond_t *start_condition;
    int *start;
    size_t task_count;
    int failed;
};

static void increment(void *opaque)
{
    struct counter *counter = (struct counter *)opaque;
    (void)pthread_mutex_lock(&counter->mutex);
    ++counter->value;
    (void)pthread_mutex_unlock(&counter->mutex);
}

static void slow_increment(void *opaque)
{
    const struct timespec delay = { 0, 50000000L };
    (void)nanosleep(&delay, NULL);
    increment(opaque);
}

static void *submit_tasks(void *opaque)
{
    struct producer *producer = (struct producer *)opaque;
    size_t i;

    (void)pthread_mutex_lock(producer->start_mutex);
    while (!*producer->start) {
        (void)pthread_cond_wait(producer->start_condition, producer->start_mutex);
    }
    (void)pthread_mutex_unlock(producer->start_mutex);

    for (i = 0U; i < producer->task_count; ++i) {
        thread_pool_status_t status;
        do {
            status = thread_pool_submit(producer->pool, increment,
                                        producer->counter);
            if (status == THREAD_POOL_QUEUE_FULL) {
                const struct timespec delay = { 0, 100000L };
                (void)nanosleep(&delay, NULL);
            }
        } while (status == THREAD_POOL_QUEUE_FULL);
        if (status != THREAD_POOL_OK) {
            producer->failed = 1;
            break;
        }
    }
    return NULL;
}

static int test_invalid_arguments(void)
{
    thread_pool_t *pool = (thread_pool_t *)1;
    CHECK(thread_pool_create(NULL, 1U, 1U) == THREAD_POOL_INVALID_ARGUMENT);
    CHECK(thread_pool_create(&pool, 0U, 1U) == THREAD_POOL_INVALID_ARGUMENT);
    CHECK(pool == NULL);
    CHECK(thread_pool_create(&pool, 1U, 0U) == THREAD_POOL_INVALID_ARGUMENT);
    CHECK(pool == NULL);
    CHECK(thread_pool_submit(NULL, increment, NULL) == THREAD_POOL_INVALID_ARGUMENT);
    CHECK(thread_pool_submit((thread_pool_t *)1, NULL, NULL) == THREAD_POOL_INVALID_ARGUMENT);
    thread_pool_destroy(NULL);
    return 0;
}

static int test_submit_and_drain(void)
{
    enum { TASK_COUNT = 1000 };
    struct counter counter;
    thread_pool_t *pool = NULL;
    size_t i;

    counter.value = 0U;
    CHECK(pthread_mutex_init(&counter.mutex, NULL) == 0);
    CHECK(thread_pool_create(&pool, 4U, TASK_COUNT) == THREAD_POOL_OK);
    for (i = 0U; i < TASK_COUNT; ++i) {
        CHECK(thread_pool_submit(pool, increment, &counter) == THREAD_POOL_OK);
    }
    thread_pool_destroy(pool);
    CHECK(counter.value == TASK_COUNT);
    CHECK(pthread_mutex_destroy(&counter.mutex) == 0);
    return 0;
}

static int test_concurrent_submitters(void)
{
    enum { PRODUCER_COUNT = 8, TASKS_PER_PRODUCER = 500 };
    struct counter counter;
    struct producer producers[PRODUCER_COUNT];
    pthread_t threads[PRODUCER_COUNT];
    pthread_mutex_t start_mutex;
    pthread_cond_t start_condition;
    thread_pool_t *pool = NULL;
    int start = 0;
    size_t i;

    counter.value = 0U;
    CHECK(pthread_mutex_init(&counter.mutex, NULL) == 0);
    CHECK(pthread_mutex_init(&start_mutex, NULL) == 0);
    CHECK(pthread_cond_init(&start_condition, NULL) == 0);
    CHECK(thread_pool_create(&pool, 4U, 16U) == THREAD_POOL_OK);

    for (i = 0U; i < PRODUCER_COUNT; ++i) {
        producers[i].pool = pool;
        producers[i].counter = &counter;
        producers[i].start_mutex = &start_mutex;
        producers[i].start_condition = &start_condition;
        producers[i].start = &start;
        producers[i].task_count = TASKS_PER_PRODUCER;
        producers[i].failed = 0;
        CHECK(pthread_create(&threads[i], NULL, submit_tasks, &producers[i]) == 0);
    }

    CHECK(pthread_mutex_lock(&start_mutex) == 0);
    start = 1;
    CHECK(pthread_cond_broadcast(&start_condition) == 0);
    CHECK(pthread_mutex_unlock(&start_mutex) == 0);

    for (i = 0U; i < PRODUCER_COUNT; ++i) {
        CHECK(pthread_join(threads[i], NULL) == 0);
        CHECK(!producers[i].failed);
    }

    thread_pool_destroy(pool);
    CHECK(counter.value == (size_t)PRODUCER_COUNT * TASKS_PER_PRODUCER);
    CHECK(pthread_cond_destroy(&start_condition) == 0);
    CHECK(pthread_mutex_destroy(&start_mutex) == 0);
    CHECK(pthread_mutex_destroy(&counter.mutex) == 0);
    return 0;
}

static int test_full_queue(void)
{
    struct counter counter;
    thread_pool_t *pool = NULL;
    size_t attempts;
    int saw_full = 0;
    const struct timespec delay = { 0, 1000000L };

    counter.value = 0U;
    CHECK(pthread_mutex_init(&counter.mutex, NULL) == 0);
    CHECK(thread_pool_create(&pool, 1U, 1U) == THREAD_POOL_OK);

    /* Keep the worker busy, then occupy the sole waiting slot. */
    CHECK(thread_pool_submit(pool, slow_increment, &counter) == THREAD_POOL_OK);
    for (attempts = 0U; attempts < 1000U; ++attempts) {
        if (thread_pool_pending(pool) == 0U) {
            break;
        }
        (void)nanosleep(&delay, NULL);
    }
    CHECK(thread_pool_submit(pool, slow_increment, &counter) == THREAD_POOL_OK);
    CHECK(thread_pool_submit(pool, slow_increment, &counter) == THREAD_POOL_QUEUE_FULL);
    saw_full = 1;
    CHECK(saw_full);

    thread_pool_destroy(pool);
    CHECK(counter.value == 2U);
    CHECK(pthread_mutex_destroy(&counter.mutex) == 0);
    return 0;
}

static int test_status_strings(void)
{
    CHECK(thread_pool_status_string(THREAD_POOL_OK) != NULL);
    CHECK(thread_pool_status_string((thread_pool_status_t)999) != NULL);
    return 0;
}

int main(void)
{
    CHECK(test_invalid_arguments() == 0);
    CHECK(test_submit_and_drain() == 0);
    CHECK(test_concurrent_submitters() == 0);
    CHECK(test_full_queue() == 0);
    CHECK(test_status_strings() == 0);
    (void)puts("All thread pool tests passed.");
    return 0;
}
