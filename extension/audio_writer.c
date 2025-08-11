#include "audio.h"

#include <stdlib.h>

#include "constants.h"
#include "types.h"

void audio_writer(track track, input_data data) {
    float *samples = track->data;
    int num_values = track->metadata.num_values;
    float *input_buffer = data->input_buffer;

    if (track->metadata.sample_rate != DEFAULT_SAMPLE_RATE) {
        printf("\n\nsample rate of track is not 44100\n\n");
    }

    int i = 0;
    for (; i < num_values; i++) {
        input_buffer[(i + data->write_index) % data->buffer_size] = samples[i];
    }

    data->write_index = (data->write_index + i) % data->buffer_size;
}

void audio_clear(input_data data) {
    float *input_buffer = data->input_buffer;
    for (int i = 0; i < data->buffer_size; i++) {
        input_buffer[(i + data->write_index) % data->buffer_size] = 0;
    }
}
