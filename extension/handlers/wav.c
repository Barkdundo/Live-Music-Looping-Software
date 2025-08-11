#include "wav.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../constants.h"
#include "../track.h"
#include "../types.h"

#define BYTE_SIZE 8
#define MAX_LINE_LEN 50
#define equal_strs(str1, str2) (strcmp(str1, str2) == 0)

static char *key_strs[] = {"a",   "a_m", "a_sharp", "a_sharp_m", "b",
                           "b_m", "c",   "c_m",     "c_sharp",   "c_sharp_m",
                           "d",   "d_m", "d_sharp", "d_sharp_m", "e",
                           "e_m", "f",   "f_m",     "f_sharp",   "f_sharp_m",
                           "g",   "g_m", "g_sharp", "g_sharp_m", "na"};

static int strs_to_keys[] = {
    A,       A_M,       A_SHARP, A_SHARP_M, B,       B_M,       C,        C_M,
    C_SHARP, C_SHARP_M, D,       D_M,       D_SHARP, D_SHARP_M, E,        E_M,
    F,       F_M,       F_SHARP, F_SHARP_M, G,       G_M,       G_SHARP,  G_SHARP_M, NA};

int find_key(char *key) {
    int size_of_sk_map = sizeof(strs_to_keys) / sizeof(int);
    for (int i = 0; i < size_of_sk_map; i++) {
        if (strcmp(key, key_strs[i]) == 0) {
            return strs_to_keys[i];
        }
    }
    return -1;
}

static void f_printf(const char *format, ...) {
    FILE *file = fopen("debugwav.log", "a");

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

#ifndef TEST_BUILD
static
#endif
    uint16_t
    read16_file_le(FILE *file) {
    uint8_t bytes[2];
    size_t num_members = sizeof(uint16_t) / sizeof(uint8_t);
    fread(bytes, sizeof(uint8_t), num_members, file);
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << BYTE_SIZE);
}

#ifndef TEST_BUILD
static
#endif
    uint32_t
    read32_file_le(FILE *file) {
    uint8_t bytes[4];
    size_t num_members = sizeof(uint32_t) / sizeof(uint8_t);
    fread(bytes, sizeof(uint8_t), num_members, file);
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << BYTE_SIZE) |
           ((uint32_t)bytes[2] << (BYTE_SIZE * 2)) |
           ((uint32_t)bytes[3] << (BYTE_SIZE * 3));
}

#ifndef TEST_BUILD
static
#endif
    int
    read_num_chars(char *dest, size_t num, FILE *file) {
    // pre: dest has space for num + 1 chars
    size_t num_read = fread(dest, sizeof(char), num, file);
    if (num_read != num) {
        return 0;
    }
    dest[num] = '\0';
    return 1;
}

struct metadata read_parse_metadata(FILE *meta_file) {
    // we will assume that the metadata file is well formed, i.e. all metadata
    // fields are accounted for and there is an entry for each, even if
    // the field is not applicable to the sample there will be some entry
    char line[MAX_LINE_LEN];
    struct metadata meta_data;
    while (fgets(line, MAX_LINE_LEN, meta_file) != NULL) {
        int len = strlen(line);
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        char *token = strtok(line, " ");
        char *value = strtok(NULL, " ");
        int cast_value = (int)(strtol(value, NULL, 0));
        if (equal_strs(token, "bpm:")) {
            meta_data.bpm = cast_value;
        } else if (equal_strs(token, "key:")) {
            meta_data.key = find_key(value); // add 1 to avoid " "
        } else if (equal_strs(token, "loop_length:")) {
            meta_data.loop_length = cast_value;
        }
    }

    return meta_data;
}

track wav_to_track(wav *wav_data, struct metadata meta_data) {
    // required continually
    track new_track = malloc(sizeof(struct track));
    assert(new_track != NULL);

    int num_values = wav_data->header.data_size / sizeof(uint16_t);

    // PCM integer conversion to IEEE 754 float
    assert(wav_data->header.audio_format == PCM_IND);
    // required continually
    float *float_data = malloc(num_values * sizeof(float));
    uint16_t *pcm_data = wav_data->data;
    for (int i = 0; i < num_values; i++) {
        // cast to int16_t to preserve sign
        float_data[i] = (float)((int16_t)pcm_data[i]) / 32768.0f;
    }

    meta_data.audio_format = FLOAT_IND;
    meta_data.sample_rate = wav_data->header.sample_rate;
    meta_data.num_values = num_values;
    meta_data.num_channels = wav_data->header.num_channels;
    meta_data.num_frames = meta_data.num_values / meta_data.num_channels;

    new_track->data = float_data;
    new_track->metadata = meta_data;

    normalise_track(new_track);
    free(wav_data->data);
    free(wav_data);
    return new_track;
}

track read_parse_wav(FILE *data_file, FILE *meta_file) {
    wav_header header;
    read_num_chars(header.file_type_id, 4, data_file);
    assert(equal_strs(header.file_type_id, "RIFF"));

    header.file_size = read32_file_le(data_file);

    read_num_chars(header.file_format, 4, data_file);
    assert(equal_strs(header.file_format, "WAVE"));
    read_num_chars(header.format_marker, 4, data_file);
    assert(equal_strs(header.format_marker, "fmt "));

    header.chunk_size = read32_file_le(data_file);
    header.audio_format = read16_file_le(data_file);
    header.num_channels = read16_file_le(data_file);
    header.sample_rate = read32_file_le(data_file);
    header.byte_rate = read32_file_le(data_file);
    header.bytes_per_block = read16_file_le(data_file);
    header.bits_per_sample = read16_file_le(data_file);

    char chunk_id[5];
    size_t chunk_size;

    while (1) { // find data indicator
        fread(chunk_id, sizeof(char), 4, data_file);
        chunk_id[4] = '\0';
        chunk_size = read32_file_le(data_file);
        if (equal_strs(chunk_id, "data")) {
            strcpy(header.data_id, chunk_id);
            header.data_size = chunk_size;
            break;
        }

        uint32_t skip = (chunk_size + 1) & ~1u;
        if (fseek(data_file, skip, SEEK_CUR) != 0) {
            perror("fseeking");
            exit(1);
        }
    }

    int num_values = header.data_size / sizeof(uint16_t);
    // freed in wav_to_track
    uint16_t *data = malloc(num_values * sizeof(uint16_t));
    assert(data != NULL);

    for (int i = 0; i < num_values; i++) {
        data[i] = read16_file_le(data_file);
    }

    // freed in wav_to_track
    wav *read_wav = malloc(sizeof(wav));
    assert(read_wav != NULL);
    read_wav->header = header;
    read_wav->data = data;

    struct metadata read_meta = read_parse_metadata(meta_file);

    return wav_to_track(read_wav, read_meta);
}
