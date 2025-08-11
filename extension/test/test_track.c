#include "test.h"

#include <portaudio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../audio.h"
#include "../audio_writer.h"
#include "../handlers/wav.h"
#include "../main.h"
#include "../track.h"
#include "../types.h"
#include "test.h"

void test_set_bpm(void) {
    input_data data = generate_input_data();
    s.input_data = data;

    PaStream *stream;
    int err = play_audio(data, &stream);
    if (err) {
        printf("Audio initialisation failed\n");
        return;
    }

    music_pair pairs[MAX_LOADED_TRACKS];
    char filenames[][MAX_FILENAME_LEN] = {"guitar_loop_01_120_cm.wav",
                                          "guitar_loop_01_120_cm.dat"};
    pair_files(2, filenames, pairs);

    track track_data = read_parse_wav(pairs[0].data, pairs[0].metadata);
    set_bpm(track_data, 200);
    audio_writer(track_data, data);

    uint64_t i = 1000000000000;
    while (i > 0) {
        i--;
    }

    end_audio(stream);

    free(data->input_buffer);
    free(data);
}

void test_change_key(void) {
    input_data data = generate_input_data();
    s.input_data = data;

    PaStream *stream;
    int err = play_audio(data, &stream);
    if (err) {
        printf("Audio initialisation failed\n");
        return;
    }

    music_pair pairs[MAX_LOADED_TRACKS];
    char filenames[][MAX_FILENAME_LEN] = {"guitar_loop_01_120_cm.wav",
                                          "guitar_loop_01_120_cm.dat"};
    pair_files(2, filenames, pairs);

    track track_data = read_parse_wav(pairs[0].data, pairs[0].metadata);
    key_change(track_data, 19);
    audio_writer(track_data, data);

    uint64_t i = 1000000000000;
    while (i > 0) {
        i--;
    }

    end_audio(stream);

    free(data->input_buffer);
    free(data);
}