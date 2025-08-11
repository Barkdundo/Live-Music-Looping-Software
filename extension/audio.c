#include "audio.h"
#include "constants.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_PI 3.14159265358979323846F
#define FRAMES_PER_BAR ((DEFAULT_SAMPLE_RATE * 60 * BEATS_PER_BAR) / DEFAULT_BPM)

static int
paCallback(const void *input_buffer,  // live input stream, not needed for loops
           void *output_buffer,       // location of where function plays from
           unsigned long frame_count, // number of frames played per cycle
           const PaStreamCallbackTimeInfo *time_info, // unneeded
           PaStreamCallbackFlags status_flags,        // unneeded
           void *user_data) // contains location of our input_buffer
{
    input_data data = user_data;
    float *out = (float *)output_buffer;

    for (int i = 0; i < frame_count * 2; i++) {
        out[i] = data->input_buffer[data->read_index];
        data->read_index = (data->read_index + 1) % data->buffer_size;
    }

    return paContinue;
}

int play_audio(input_data data, PaStream **stream) {
    PaError error;

    error = Pa_Initialize();
    if (error != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(error));
        return 1;
    }

    error = Pa_OpenDefaultStream(
        stream,
        0,                   // num input streams
        2,                   // num output channels
        paFloat32,           // sample format
        DEFAULT_SAMPLE_RATE, // sample rate NOTE: COULD GO WRONG IF SAMPLE RATE
                             // IS NOT 44100!!!
        FRAMES_PER_BAR,      
        paCallback,          // call callback function
        data);               // pass in the input buffer
    if (error != paNoError) {
        fprintf(stderr, "Stream open error: %s\n", Pa_GetErrorText(error));
        Pa_Terminate();
        return 1;
    }

    error = Pa_StartStream(*stream);
    if (error != paNoError) {
        fprintf(stderr, "Stream start error: %s\n", Pa_GetErrorText(error));
        Pa_CloseStream(*stream);
        Pa_Terminate();
        return 1;
    }
    return 0;

    // keeps looping until stopped in main
}

int end_audio(PaStream *stream) {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}
