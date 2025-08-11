#include "../constants.h"
#include "../input.h"
#include "../queue.h"
#include "../types.h"
#include "test.h"
#include <curses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

volatile state s = {.tick_counter = 0, .bpm = DEFAULT_BPM};

void *output(void *_);
/*
void test_input(void)
{
    printf("Testing Input\n");

    initscr();
    cbreak();
    nodelay(stdscr, TRUE);
    s_test.tracks = calloc(MAX_LOADED_TRACKS, sizeof(track));
    s_test.registers = calloc(NUM_REGISTERS, sizeof(track));
    s_test.queue = pq_create(MAX_QUEUE_LENGTH);

    pthread_t output_loop;
    pthread_t input_loop;

    pthread_create(&output_loop, NULL, output, NULL);
    pthread_create(&input_loop, NULL, run_input, NULL);

    pthread_join(output_loop, NULL);
    pthread_join(input_loop, NULL);

    endwin();

    free(s_test.tracks);
    free(s_test.registers);

}

void *output(void*_)
{
    int s_old_q_size = s_test.queue->size;
    bool quit = false;
    struct command com;
    while(!quit)
    {
        if(s_test.queue->size != s_old_q_size)
        {
            for(int i = 0; i<s_test.queue->size; i++)
            {
                com = pq_dequeue(s_test.queue);
                printf("track int: %d tick: %d type: %d payload %d \n",
com.track, com.tick, com.type, com.payload);

            }
        }
        napms(10);
        quit = true;
    }
    return 0;
}
*/

void test_input() {

    s.tracks = calloc(MAX_LOADED_TRACKS, sizeof(track));
    s.registers = calloc(NUM_REGISTERS, sizeof(track));
    s.queue = pq_create(MAX_QUEUE_LENGTH);

    for (int i = 0; i < NUM_REGISTERS; ++i) {
        track t = calloc(1, sizeof(struct track));
        t->metadata.bpm = DEFAULT_BPM;
        t->metadata.sample_rate = DEFAULT_SAMPLE_RATE;
        t->metadata.loop_length = 1;
        t->metadata.num_channels = 1;
        s.tracks[i] = t;
        s.registers[i] = t;
    }

    initscr();
    cbreak();
    echo();
    nodelay(stdscr, TRUE);
    printw("Testing input");
    refresh();
    run_input(NULL);
    printf("here! 1\n");
    int end_size = s.queue->size;
    printf("end size is %d\n", end_size);
    endwin();

    for (int i = 0; i < end_size; i++) {
        printf("here 2 at %d\n", i);
        struct command com = pq_dequeue(s.queue);
        printf("\n track: %d  tick: %d  type: %d  payload: %d  \n", com.track,
               com.tick, com.type, com.payload);
    }
}
