#pragma once

// main constants

#define MAX_FILENAME_LEN 100
#define MAX_LOADED_TRACKS 200
#define NUM_REGISTERS 10
#define MAX_QUEUE_LENGTH 100

#define METADATA_EXTENSION ".dat"
#define SOUND_DIR "sounds/"

#define DEFAULT_BPM 145

#define DEFAULT_SAMPLE_RATE 44100

#define BUFFER_SIZE                                                            \
    (int)(DEFAULT_SAMPLE_RATE * (240.0f / DEFAULT_BPM) * 4 * 2) // 2 bars

// loop constants

#define SECONDS_PER_MINUTE 60LL
#define NANOSECONDS_PER_SECOND 1000000000LL

#define MAX_QUEUE_LENGTH 100

#define BEATS_PER_BAR 4

// wav constants
#define PCM_IND 1
#define FLOAT_IND 3
