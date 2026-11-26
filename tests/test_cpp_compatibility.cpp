#include "thread_pool.h"

int main()
{
    return thread_pool_status_string(THREAD_POOL_OK) == nullptr ? 1 : 0;
}
