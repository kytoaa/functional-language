#include "remapping.h"
#include <string.h>

void enqueue_remapping_work(struct RemappingQueue *queue, struct RemappingWork work)
{
    if (queue->len == queue->cap) {
        u32 new_cap = (queue->cap == 0) ? 2 : queue->cap * 2;
        struct RemappingWork *new_ptr = alloc_mem(new_cap * sizeof(struct RemappingWork));

        u32 pre_wrap_count = (queue->ptr + queue->cap) - queue->start;
        memcpy(new_ptr, queue->start, pre_wrap_count * sizeof(struct RemappingWork));
        memcpy(new_ptr + pre_wrap_count, queue->ptr, (queue->len - pre_wrap_count) * sizeof(struct RemappingWork));

        free_mem(queue->ptr);
        queue->ptr = new_ptr;
        queue->start = new_ptr;
        queue->cap = new_cap;
    }

    struct RemappingWork *addr = queue->start + queue->len;
    if (addr >= queue->ptr + queue->cap) {
        addr = addr - queue->cap;
    }

    queue->len += 1;
    *addr = work;
}

struct RemappingWork dequeue_remapping_work(struct RemappingQueue *queue)
{
    if (queue->len == 0)
        panic("empty queue");

    struct RemappingWork *val = queue->start;

    queue->start += 1;
    if (queue->start == queue->ptr + queue->cap)
        queue->start = queue->ptr;

    queue->len -= 1;

    return *val;
}

void free_remapping_queue(struct RemappingQueue *queue)
{
    free_mem(queue->ptr);
    *queue = (struct RemappingQueue){};
}
