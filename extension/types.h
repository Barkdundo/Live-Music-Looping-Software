#pragma once

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "constants.h"

typedef enum {
    LOAD,
    REMOVE,
    PLAY,
    PAUSE,
    MUTE,
    VOLUME_UP,
    VOLUME_DOWN,
    BPM_SET
} command_type;

typedef enum {
    // major keys
    C,
    C_SHARP,
    D,
    D_SHARP,
    E,
    F,
    F_SHARP,
    G,
    G_SHARP,
    A,
    A_SHARP,
    B,
    // minor keys:
    C_M,
    C_SHARP_M,
    D_M,
    D_SHARP_M,
    E_M,
    F_M,
    F_SHARP_M,
    G_M,
    G_SHARP_M,
    A_M,
    A_SHARP_M,
    B_M,
    // not applicable
    NA
} key;

struct command {
    int track;
    int tick;
    int type;
    int payload;
};

typedef struct {
    FILE *data;
    FILE *metadata;
    char name[MAX_FILENAME_LEN];
} music_pair;

struct metadata {
    int bpm;
    key key;
    int sample_rate;
    int loop_length;
    int num_channels;
    int audio_format;
    int num_values;
    int num_frames;
};

typedef struct track *track;
struct track {
    int playing;
    int muted;
    float *data;
    char name[MAX_FILENAME_LEN];
    struct metadata metadata;
    int start_offset; // for the loop calculate
};

typedef struct input_data *input_data;
struct input_data {
    float *input_buffer;
    size_t buffer_size;
    int read_index;
    int write_index;
};

typedef struct command_queue *command_queue;
struct command_queue {
    struct command *data;
    int capacity;
    int size;
    pthread_mutex_t lock;
};

typedef struct {
    int tick_counter;
    int bpm;
    command_queue queue;
    track *tracks;
    track *registers;
    input_data input_data;
} state;
