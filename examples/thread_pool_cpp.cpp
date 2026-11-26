#include "thread_pool.h"

#include <cstddef>
#include <iostream>
#include <mutex>

struct task_state {
    std::mutex mutex;
    std::size_t completed;
};

static void count_task(void *opaque)
{
    task_state *state = static_cast<task_state *>(opaque);
    std::lock_guard<std::mutex> lock(state->mutex);
    ++state->completed;
}

int main()
{
    const std::size_t task_count = 1000U;
    thread_pool_t *pool = NULL;
    task_state state;
    state.completed = 0U;

    const thread_pool_status_t create_status =
        thread_pool_create(&pool, 4U, task_count);
    if (create_status != THREAD_POOL_OK) {
        std::cerr << "Could not create thread pool: "
                  << thread_pool_status_string(create_status) << '\n';
        return 1;
    }

    for (std::size_t i = 0U; i < task_count; ++i) {
        const thread_pool_status_t status =
            thread_pool_submit(pool, count_task, &state);
        if (status != THREAD_POOL_OK) {
            std::cerr << "Could not submit task: "
                      << thread_pool_status_string(status) << '\n';
            thread_pool_destroy(pool);
            return 1;
        }
    }

    thread_pool_destroy(pool);
    if (state.completed != task_count) {
        std::cerr << "Expected " << task_count << " completed tasks, got "
                  << state.completed << '\n';
        return 1;
    }

    std::cout << "Completed " << state.completed << " tasks.\n";
    return 0;
}
