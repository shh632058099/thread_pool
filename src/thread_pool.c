#include "thread_pool.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

struct thread_pool {
    pthread_t *workers;
    size_t worker_count;
    size_t workers_started;
    thread_pool_task_fn *functions;
    void **arguments;
    size_t queue_capacity;
    size_t queue_head;
    size_t queue_tail;
    size_t queue_size;
    int stopping;
    pthread_mutex_t mutex;
    pthread_cond_t work_available;
};

static void *thread_pool_worker(void *opaque)
{
    thread_pool_t *pool = (thread_pool_t *)opaque;

    for (;;) {
        thread_pool_task_fn function;
        void *arg;

        (void)pthread_mutex_lock(&pool->mutex);
        while (pool->queue_size == 0U && !pool->stopping) {
            (void)pthread_cond_wait(&pool->work_available, &pool->mutex);
        }

        if (pool->queue_size == 0U && pool->stopping) {
            (void)pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }

        function = pool->functions[pool->queue_head];
        arg = pool->arguments[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1U) % pool->queue_capacity;
        --pool->queue_size;
        (void)pthread_mutex_unlock(&pool->mutex);

        function(arg);
    }
}

static void thread_pool_release(thread_pool_t *pool)
{
    free(pool->arguments);
    free(pool->functions);
    free(pool->workers);
    free(pool);
}

thread_pool_status_t thread_pool_create(thread_pool_t **out_pool,
                                        size_t worker_count,
                                        size_t queue_capacity)
{
    thread_pool_t *pool;
    size_t i;

    if (out_pool == NULL) {
        return THREAD_POOL_INVALID_ARGUMENT;
    }
    *out_pool = NULL;
    if (worker_count == 0U || queue_capacity == 0U ||
        worker_count > SIZE_MAX / sizeof(pthread_t) ||
        queue_capacity > SIZE_MAX / sizeof(thread_pool_task_fn) ||
        queue_capacity > SIZE_MAX / sizeof(void *)) {
        return THREAD_POOL_INVALID_ARGUMENT;
    }

    pool = (thread_pool_t *)calloc(1U, sizeof(*pool));
    if (pool == NULL) {
        return THREAD_POOL_OUT_OF_MEMORY;
    }
    pool->worker_count = worker_count;
    pool->queue_capacity = queue_capacity;
    pool->workers = (pthread_t *)calloc(worker_count, sizeof(*pool->workers));
    pool->functions = (thread_pool_task_fn *)calloc(queue_capacity,
                                                    sizeof(*pool->functions));
    pool->arguments = (void **)calloc(queue_capacity, sizeof(*pool->arguments));
    if (pool->workers == NULL || pool->functions == NULL ||
        pool->arguments == NULL) {
        thread_pool_release(pool);
        return THREAD_POOL_OUT_OF_MEMORY;
    }

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        thread_pool_release(pool);
        return THREAD_POOL_SYSTEM_ERROR;
    }
    if (pthread_cond_init(&pool->work_available, NULL) != 0) {
        (void)pthread_mutex_destroy(&pool->mutex);
        thread_pool_release(pool);
        return THREAD_POOL_SYSTEM_ERROR;
    }

    for (i = 0U; i < worker_count; ++i) {
        if (pthread_create(&pool->workers[i], NULL, thread_pool_worker, pool) != 0) {
            (void)pthread_mutex_lock(&pool->mutex);
            pool->stopping = 1;
            (void)pthread_cond_broadcast(&pool->work_available);
            (void)pthread_mutex_unlock(&pool->mutex);
            while (pool->workers_started > 0U) {
                --pool->workers_started;
                (void)pthread_join(pool->workers[pool->workers_started], NULL);
            }
            (void)pthread_cond_destroy(&pool->work_available);
            (void)pthread_mutex_destroy(&pool->mutex);
            thread_pool_release(pool);
            return THREAD_POOL_SYSTEM_ERROR;
        }
        ++pool->workers_started;
    }

    *out_pool = pool;
    return THREAD_POOL_OK;
}

thread_pool_status_t thread_pool_submit(thread_pool_t *pool,
                                        thread_pool_task_fn function,
                                        void *arg)
{
    if (pool == NULL || function == NULL) {
        return THREAD_POOL_INVALID_ARGUMENT;
    }
    if (pthread_mutex_lock(&pool->mutex) != 0) {
        return THREAD_POOL_SYSTEM_ERROR;
    }
    if (pool->stopping) {
        (void)pthread_mutex_unlock(&pool->mutex);
        return THREAD_POOL_SHUTTING_DOWN;
    }
    if (pool->queue_size == pool->queue_capacity) {
        (void)pthread_mutex_unlock(&pool->mutex);
        return THREAD_POOL_QUEUE_FULL;
    }

    pool->functions[pool->queue_tail] = function;
    pool->arguments[pool->queue_tail] = arg;
    pool->queue_tail = (pool->queue_tail + 1U) % pool->queue_capacity;
    ++pool->queue_size;
    (void)pthread_cond_signal(&pool->work_available);
    (void)pthread_mutex_unlock(&pool->mutex);
    return THREAD_POOL_OK;
}

void thread_pool_destroy(thread_pool_t *pool)
{
    size_t i;

    if (pool == NULL) {
        return;
    }

    (void)pthread_mutex_lock(&pool->mutex);
    pool->stopping = 1;
    (void)pthread_cond_broadcast(&pool->work_available);
    (void)pthread_mutex_unlock(&pool->mutex);

    for (i = 0U; i < pool->workers_started; ++i) {
        (void)pthread_join(pool->workers[i], NULL);
    }

    (void)pthread_cond_destroy(&pool->work_available);
    (void)pthread_mutex_destroy(&pool->mutex);
    thread_pool_release(pool);
}

size_t thread_pool_pending(thread_pool_t *pool)
{
    size_t pending;

    if (pool == NULL || pthread_mutex_lock(&pool->mutex) != 0) {
        return 0U;
    }
    pending = pool->queue_size;
    (void)pthread_mutex_unlock(&pool->mutex);
    return pending;
}

const char *thread_pool_status_string(thread_pool_status_t status)
{
    switch (status) {
    case THREAD_POOL_OK:
        return "success";
    case THREAD_POOL_INVALID_ARGUMENT:
        return "invalid argument";
    case THREAD_POOL_OUT_OF_MEMORY:
        return "out of memory";
    case THREAD_POOL_QUEUE_FULL:
        return "task queue full";
    case THREAD_POOL_SHUTTING_DOWN:
        return "pool is shutting down";
    case THREAD_POOL_SYSTEM_ERROR:
        return "system error";
    default:
        return "unknown status";
    }
}
