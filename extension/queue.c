#include "queue.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "types.h"

static void pq_fix_insertion(command_queue queue, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        struct command parent_command = queue->data[parent];
        struct command index_command = queue->data[index];
        if (parent_command.tick <= index_command.tick) {
            break;
        }
        queue->data[parent] = index_command;
        queue->data[index] = parent_command;
        index = parent;
    }
}

static void pq_fix_deletion(command_queue queue, int index) {
    while (1) {
        int left = (2 * index) + 1;
        int right = (2 * index) + 2;
        int smallest = index;

        if (left < queue->size &&
            queue->data[left].tick < queue->data[smallest].tick) {
            smallest = left;
        }
        if (right < queue->size &&
            queue->data[right].tick < queue->data[smallest].tick) {
            smallest = right;
        }

        if (smallest == index) {
            break;
        }

        struct command index_command = queue->data[index];
        queue->data[index] = queue->data[smallest];
        queue->data[smallest] = index_command;

        index = smallest;
    }
}

command_queue pq_create(int capacity) {
    command_queue queue = malloc(sizeof(struct command_queue));
    assert(queue != NULL);
    queue->data = malloc(sizeof(struct command) * capacity);
    assert(queue->data != NULL);
    queue->capacity = capacity;
    queue->size = 0;
    pthread_mutex_init(&queue->lock, NULL);
    return queue;
}

void pq_resize(command_queue queue) {
    queue->capacity *= 2;
    queue->data =
        realloc(queue->data, sizeof(struct command) * queue->capacity);
    assert(queue->data != NULL);
}

void pq_enqueue(command_queue queue, struct command command) {
    pthread_mutex_lock(&queue->lock);
    if (queue->size == queue->capacity) {
        pq_resize(queue);
    }
    queue->data[queue->size] = command;
    pq_fix_insertion(queue, queue->size);
    queue->size++;
    pthread_mutex_unlock(&queue->lock);
}

struct command pq_peek(command_queue queue) {
    pthread_mutex_lock(&queue->lock);
    assert(queue->size > 0);
    pthread_mutex_unlock(&queue->lock);
    return queue->data[0];
}

struct command pq_dequeue(command_queue queue) {
    pthread_mutex_lock(&queue->lock);
    assert(queue->size > 0);
    struct command result = queue->data[0];

    queue->size--;
    queue->data[0] = queue->data[queue->size];
    pq_fix_deletion(queue, 0);

    pthread_mutex_unlock(&queue->lock);
    return result;
}

void pq_delete(command_queue queue) {
    pthread_mutex_destroy(&queue->lock);
    free(queue->data);
    free(queue);
}
