#pragma once

#include <stdint.h>
#include <stdio.h>

#include "../types.h"

#ifdef TEST_BUILD
extern int read_num_chars(char *, size_t, FILE *);
extern uint16_t read16_file_le(FILE *);
extern uint32_t read32_file_le(FILE *);
#endif

typedef struct {
    char file_type_id[5]; // RIFF
    uint32_t file_size;
    char file_format[5];   // WAVE
    char format_marker[5]; // fmt
    uint32_t chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;     // in Hz
    uint32_t byte_rate;       // number of bytes to read per second
    uint16_t bytes_per_block; // block align
    uint16_t bits_per_sample;
    char data_id[5]; // "data"
    uint32_t data_size;
} wav_header;
// precondition that all input .wav files use audio format 1 (PCM integer) and
// use 16 bits per sample

typedef struct {
    wav_header header;
    uint16_t *data;
} wav;

extern track read_parse_wav(FILE *, FILE *);