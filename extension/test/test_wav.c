#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NDEBUG
#include <assert.h>

// we include the .c files to use the helpers defined within,
// without exposing them as part of their public interface in
// their corresponding .h file
#include "../constants.h"
#include "../handlers/wav.h"
#include "../main.h"
#include "../types.h"

#define equal_strs(str1, str2) (strcmp(str1, str2) == 0)

// copied section of only the read part of the file, so we can check that
// the file is read correctly, as reading and parsing are done in the same
// function
static wav *read_wav(FILE *data_file, FILE *meta_file) {
    wav_header header;
    read_num_chars(header.file_type_id, 4, data_file);
    printf("%s\n", header.file_type_id);
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
    uint16_t *data = malloc(num_values * sizeof(uint16_t));
    assert(data != NULL);

    for (int i = 0; i < num_values; i++) {
        data[i] = read16_file_le(data_file);
    }

    wav *read_wav = malloc(sizeof(wav));
    assert(read_wav != NULL);
    read_wav->header = header;
    read_wav->data = data;
    return read_wav;
}

void test_wav(void) {
    music_pair pairs[MAX_LOADED_TRACKS];
    char filenames[][MAX_FILENAME_LEN] = {"drum_loop_01_100.wav",
                                          "drum_loop_01_100.dat"};
    pair_files(2, filenames, pairs);

    wav *test_read_wav = read_wav(pairs[0].data, pairs[0].metadata);

    printf("wav reading tests here:\n");
    printf("file_type_id: %s\n", test_read_wav->header.file_type_id);
    printf("file_size: %u\n", test_read_wav->header.file_size);
    printf("file_format: %s\n", test_read_wav->header.file_format);
    printf("format_marker: %s\n", test_read_wav->header.format_marker);
    printf("chunk_size: %u\n", test_read_wav->header.chunk_size);
    printf("audio_format: %u\n", test_read_wav->header.audio_format);
    printf("num_channels: %u\n", test_read_wav->header.num_channels);
    printf("sample_rate: %u\n", test_read_wav->header.sample_rate);
    printf("byte_rate: %u\n", test_read_wav->header.byte_rate);
    printf("bytes_per_block: %u\n", test_read_wav->header.bytes_per_block);
    printf("bits_per_sample: %u\n", test_read_wav->header.bits_per_sample);
    printf("data_id: %s\n", test_read_wav->header.data_id);
    printf("data_size: %u\n", test_read_wav->header.data_size);

    printf("metadata parsing tests here:\n");
    rewind(pairs[0].data);
    rewind(pairs[0].metadata);
    track parsed_track = read_parse_wav(pairs[0].data, pairs[0].metadata);
    printf("bpm: %d\n", parsed_track->metadata.bpm);
    printf("key: %d\n", parsed_track->metadata.key);
    printf("sample_rate: %d\n", parsed_track->metadata.sample_rate);
    printf("loop_length: %d\n", parsed_track->metadata.loop_length);
    printf("audio_format: %d\n", parsed_track->metadata.audio_format);
    printf("num_values: %d\n", parsed_track->metadata.num_values);
    printf("num_frames: %d\n", parsed_track->metadata.num_frames);
}