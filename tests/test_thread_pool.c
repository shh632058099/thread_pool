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
    CHECK(test_full_queue() == 0);
    CHECK(test_status_strings() == 0);
    (void)puts("All thread pool tests passed.");
    return 0;
}
