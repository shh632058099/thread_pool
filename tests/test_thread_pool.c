#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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

struct task_gate {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    size_t started;
    size_t active;
    size_t max_active;
    int released;
};

static void increment(void *opaque)
{
    struct counter *counter = (struct counter *)opaque;
    (void)pthread_mutex_lock(&counter->mutex);
    ++counter->value;
    (void)pthread_mutex_unlock(&counter->mutex);
}

static void gated_task(void *opaque)
{
    struct task_gate *gate = (struct task_gate *)opaque;
    (void)pthread_mutex_lock(&gate->mutex);
    ++gate->started;
    ++gate->active;
    if (gate->active > gate->max_active) {
        gate->max_active = gate->active;
    }
    (void)pthread_cond_broadcast(&gate->condition);
    while (!gate->released) {
        (void)pthread_cond_wait(&gate->condition, &gate->mutex);
    }
    --gate->active;
    (void)pthread_mutex_unlock(&gate->mutex);
}

static int gate_init(struct task_gate *gate)
{
    gate->started = 0U;
    gate->active = 0U;
    gate->max_active = 0U;
    gate->released = 0;
    if (pthread_mutex_init(&gate->mutex, NULL) != 0) {
        return 0;
    }
    if (pthread_cond_init(&gate->condition, NULL) != 0) {
        (void)pthread_mutex_destroy(&gate->mutex);
        return 0;
    }
    return 1;
}

static void gate_wait_for_started(struct task_gate *gate, size_t count)
{
    (void)pthread_mutex_lock(&gate->mutex);
    while (gate->started < count) {
        (void)pthread_cond_wait(&gate->condition, &gate->mutex);
    }
    (void)pthread_mutex_unlock(&gate->mutex);
}

static void gate_release(struct task_gate *gate)
{
    (void)pthread_mutex_lock(&gate->mutex);
    gate->released = 1;
    (void)pthread_cond_broadcast(&gate->condition);
    (void)pthread_mutex_unlock(&gate->mutex);
}

static void gate_destroy(struct task_gate *gate)
{
    (void)pthread_cond_destroy(&gate->condition);
    (void)pthread_mutex_destroy(&gate->mutex);
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
    struct task_gate gate;
    thread_pool_t *pool = NULL;

    CHECK(gate_init(&gate));
    CHECK(thread_pool_create(&pool, 1U, 1U) == THREAD_POOL_OK);
    CHECK(thread_pool_submit(pool, gated_task, &gate) == THREAD_POOL_OK);
    gate_wait_for_started(&gate, 1U);
    CHECK(thread_pool_pending(pool) == 0U);
    CHECK(thread_pool_submit(pool, gated_task, &gate) == THREAD_POOL_OK);
    CHECK(thread_pool_pending(pool) == 1U);
    CHECK(thread_pool_submit(pool, gated_task, &gate) == THREAD_POOL_QUEUE_FULL);

    gate_release(&gate);
    thread_pool_destroy(pool);
    CHECK(gate.started == 2U);
    gate_destroy(&gate);
    return 0;
}

static int test_concurrent_callbacks(void)
{
    struct task_gate gate;
    thread_pool_t *pool = NULL;

    CHECK(gate_init(&gate));
    CHECK(thread_pool_create(&pool, 2U, 2U) == THREAD_POOL_OK);
    CHECK(thread_pool_submit(pool, gated_task, &gate) == THREAD_POOL_OK);
    CHECK(thread_pool_submit(pool, gated_task, &gate) == THREAD_POOL_OK);
    gate_wait_for_started(&gate, 2U);
    gate_release(&gate);
    thread_pool_destroy(pool);
    CHECK(gate.max_active == 2U);
    gate_destroy(&gate);
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
    CHECK(test_concurrent_callbacks() == 0);
    CHECK(test_status_strings() == 0);
    (void)puts("All thread pool tests passed.");
    return 0;
}
