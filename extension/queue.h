#pragma once

#include "types.h"

command_queue pq_create(int capacity);

extern void pq_enqueue(command_queue, struct command);

extern void pq_resize(command_queue);

extern struct command pq_peek(command_queue);

extern struct command pq_dequeue(command_queue);
