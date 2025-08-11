#include "../queue.h"
#include "test.h"

void test_queue(void) {
    command_queue queue = pq_create(1);
    struct command command_0 = {0, 5, 0};
    struct command command_1 = {1, 3, 0};
    struct command command_2 = {2, 2, 0};
    struct command command_3 = {3, 7, 0};
    struct command command_4 = {4, 5, 0};

    printf("\n\nTesting Enqueue\n\n");
    pq_enqueue(queue, command_0);
    pq_enqueue(queue, command_1);
    pq_enqueue(queue, command_2);
    pq_enqueue(queue, command_3);
    pq_enqueue(queue, command_4);

    if (queue->size != 5) {
        printf("size is not 5\n");
        return;
    }

    printf("\n\nTesting Order\n\n");
    for (int i = 0; i < 5; i++) {
        printf("item %d has tick %d\n", i, queue->data[i].tick);
    }

    printf("\n\nTesting Dequeue\n\n");
    for (int i = 0; i < 5; i++) {
        struct command command = pq_dequeue(queue);
        printf("removed command with tick %d\n", command.tick);
    }
    if (queue->size != 0) {
        printf("size is not 0\n");
    }
}
