#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct thread_pool thread_pool_t;

/* Returned by the pool API. */
typedef enum thread_pool_status {
    THREAD_POOL_OK = 0,
    THREAD_POOL_INVALID_ARGUMENT,
    THREAD_POOL_OUT_OF_MEMORY,
    THREAD_POOL_QUEUE_FULL,
    THREAD_POOL_SHUTTING_DOWN,
    THREAD_POOL_SYSTEM_ERROR
} thread_pool_status_t;

/* A task function receives the opaque argument supplied to submit(). */
typedef void (*thread_pool_task_fn)(void *arg);

/*
 * Create a pool with worker_count workers and a bounded queue of queue_capacity
 * pending tasks. Both values must be greater than zero. On success, *out_pool
 * receives the created pool; on failure it is set to NULL.
 */
thread_pool_status_t thread_pool_create(thread_pool_t **out_pool,
                                        size_t worker_count,
                                        size_t queue_capacity);

/*
 * Submit a task. This call never blocks waiting for queue space: it returns
 * THREAD_POOL_QUEUE_FULL if all queue slots are occupied. The pool takes no
 * ownership of arg; callers are responsible for its lifetime.
 */
thread_pool_status_t thread_pool_submit(thread_pool_t *pool,
                                        thread_pool_task_fn function,
                                        void *arg);

/*
 * Gracefully stop the pool: reject future submissions, finish queued tasks,
 * and join all workers. Safe to call once; do not call concurrently or from
 * one of this pool's worker threads. Destroys pool resources and invalidates
 * pool, so the caller must not use it afterward.
 */
void thread_pool_destroy(thread_pool_t *pool);

/* Return the number of tasks currently waiting in the queue. */
size_t thread_pool_pending(thread_pool_t *pool);

/* Return a human-readable status name. */
const char *thread_pool_status_string(thread_pool_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* THREAD_POOL_H */
