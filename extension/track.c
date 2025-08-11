#include "track.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "cpplib/soundtouch_wrapper.h"
#include "types.h"

#define STD_VOL_UP 1.5f
#define STD_VOL_DOWN 0.67f
#define BEATS_PER_BAR 4
#define FRAMES_PER_CHUNK 1024

void normalise_track(track track_data) {
    int num_values = track_data->metadata.num_values;
    float max_abs_val = 0.0f;
    for (int i = 0; i < num_values; i++) {
        float curr_abs_val = fabsf(track_data->data[i]);
        if (curr_abs_val > max_abs_val) {
            max_abs_val = curr_abs_val;
        }
    }

    // normalise mixed track data if required:
    if (max_abs_val > 1.0) {
        float scale = 1.0 / max_abs_val;
        for (int i = 0; i < num_values; i++) {
            track_data->data[i] *= scale;
        }
    }
}

static void change_vol(track track_data, float amount) {
    assert(amount <= STD_VOL_UP);
    float *float_data = track_data->data;
    for (int i = 0; i < track_data->metadata.num_values; i++) {
        float_data[i] *= amount;
    }
    return;
}

#include <stdarg.h>

static void f_printf(const char *format, ...) {
    FILE *file = fopen("debugtrack.log", "a");

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

// initialises a SoundTouch object with the given track's sample rate and number
// of channels
static void *initialise_soundtouch(track track_data) {

    void *sound_touch = st_new();

    st_set_sr(sound_touch, track_data->metadata.sample_rate);
    st_set_ch(sound_touch, track_data->metadata.num_channels);

    return sound_touch;
}

// translates SoundTouch-processed data back into track data
static void receive_samples(void *sound_touch, track track_data) {

    int num_channels = track_data->metadata.num_channels;
    float buffer[FRAMES_PER_CHUNK * num_channels];
    // account for mono or stereo audio format
    int received = 0;

    float *output = malloc(sizeof(float) * track_data->metadata.num_values);
    int num_output_samples = 0;
    int cap_output_samples = track_data->metadata.num_values;

    track_data->metadata.num_values = 0;

    int num_values_received;
    do {
        received = st_recv(sound_touch, buffer, FRAMES_PER_CHUNK);
        // received is in frames
        num_values_received = received * num_channels;
        // number of frames = number of samples * number of channels

        if (num_values_received + num_output_samples > cap_output_samples) {
            cap_output_samples *= 2;
            output = realloc(output, cap_output_samples * sizeof(float));
            assert(output != NULL);
        }
        memcpy(output + num_output_samples, buffer,
               num_values_received * sizeof(float));
        num_output_samples += num_values_received;
    } while (num_values_received != 0);

    track_data->data =
        realloc(track_data->data, num_output_samples * sizeof(float));
    assert(track_data->data != NULL);

    memcpy(track_data->data, output, num_output_samples * sizeof(float));
    track_data->metadata.num_values = num_output_samples;
    track_data->metadata.num_frames = num_output_samples / num_channels;

    free(output);
}

// sets the bpm of a track to a specified number
void set_bpm(track track_data, int new_bpm) {

    void *sound_touch = initialise_soundtouch(track_data);

    int original_bpm = track_data->metadata.bpm;
    if (original_bpm == new_bpm) {
        return;
    }

    float bpm_change = (float)(new_bpm) / (float)(original_bpm);
    st_set_tempo(sound_touch, bpm_change);

    st_put(sound_touch, track_data->data, track_data->metadata.num_frames);

    receive_samples(sound_touch, track_data);
    st_flush(sound_touch);
    st_free(sound_touch);
    track_data->metadata.bpm = new_bpm;
    return;
}

// calls change_vol with the minimum of the standard volume or the max volume
// that can be applied without distorting the audio i.e. clipping
void volume_up(track track_data) {
    float *float_data = track_data->data;
    float max_abs_val = 0.0f;
    for (int i = 0; i < track_data->metadata.num_values; i++) {
        float curr_abs_val = fabsf(float_data[i]);
        if (curr_abs_val > max_abs_val) {
            max_abs_val = curr_abs_val;
        }
    }
    if (max_abs_val == 0.0f) {
        // this should technically never be entered as it implies all data
        // entries are 0
        change_vol(track_data, STD_VOL_UP);
        return;
    }
    float max_vol_up = 1.0f / max_abs_val;
    float vol_up = max_vol_up < STD_VOL_UP ? max_vol_up : STD_VOL_UP;
    change_vol(track_data, vol_up);
    return;
}

// reduces the volume of a track by a pre-specified amount
void volume_down(track track_data) { change_vol(track_data, STD_VOL_DOWN); }

void mute(track track_data) {
    track_data->muted = 1;
    return;
}

// changes the key of a track by adjusting its pitch by a calculated number of
// semitones
void key_change(track track_data, key new_key) {
    key original_key = track_data->metadata.key;

    if (original_key == NA) {
        return;
    }

    float semitone_change = (float)(new_key - original_key);

    void *sound_touch = initialise_soundtouch(track_data);

    st_set_pitch(sound_touch, semitone_change);

    st_put(sound_touch, track_data->data, track_data->metadata.num_frames);

    receive_samples(sound_touch, track_data);
    st_flush(sound_touch);
    st_free(sound_touch);

    normalise_track(track_data);
    return;
}

// returns the specified bar of a track
track get_bar(track track_data, int current_bar) {
    track new_track = calloc(1, sizeof(struct track));
    // ensure all memory is initialised

    *new_track = *track_data;

    int sample_rate = track_data->metadata.sample_rate;
    int bpm = track_data->metadata.bpm;
    int num_channels = track_data->metadata.num_channels;

    int num_frames = track_data->metadata.num_frames;
    int frames_per_bar =
        lround(((double)sample_rate * 60.0 * BEATS_PER_BAR) / (double)bpm);
    assert(current_bar * frames_per_bar < num_frames);

    int frame_offset = (int)(frames_per_bar * current_bar);

    int remaining = num_frames - frame_offset;
    if (remaining <= 0) {
        return NULL;
    }

    // now, we must pad the output in case the remaining frames is fewer than
    // the supposed number of frames
    int copy_frames = remaining < frames_per_bar ? remaining : frames_per_bar;

    new_track->metadata.num_values = frames_per_bar * num_channels;
    new_track->metadata.num_frames = frames_per_bar;
    new_track->metadata.loop_length = 1;

    new_track->data =
        malloc(sizeof(float) * num_channels * new_track->metadata.num_frames);

    memcpy(new_track->data, track_data->data + frame_offset * num_channels,
           sizeof(float) * copy_frames * num_channels);

    if (copy_frames < frames_per_bar) {
        int pad_frames = frames_per_bar - copy_frames;
        memcpy(new_track->data + copy_frames * num_channels, track_data->data,
               sizeof(float) * pad_frames * num_channels);
    }

    return new_track;
}

// mixes tracks from a given list together and normalises the result
track mix_tracks(track *tracks, int num_tracks) {
    // this function only receives one bar of tracks at a time
    // all tracks are also validated to be currently playing and not muted
    // assert(num_tracks > 0);
    if (num_tracks == 1) {
        track mixed_track = malloc(sizeof(struct track));
        float *float_data = malloc(tracks[0]->metadata.num_values * sizeof(float));

        memcpy(mixed_track, tracks[0], sizeof(struct track));
        memcpy(float_data, tracks[0]->data, tracks[0]->metadata.num_values * sizeof(float));
        mixed_track->data = float_data;
        return mixed_track;
    }
    int sample_rate = tracks[0]->metadata.sample_rate;
    int num_channels = tracks[0]->metadata.num_channels;
    int num_values = tracks[0]->metadata.num_values;

    track mixed_track = malloc(sizeof(struct track));
    assert(mixed_track != NULL);
    float *mixed_track_data = calloc(num_values, sizeof(float));
    assert(mixed_track_data != NULL);

    // assert all tracks have the same sample_rate, number of channels and
    // number of samples:
    for (int i = 0; i < num_tracks; i++) {
        track current_track = tracks[i];
        assert(current_track->metadata.sample_rate == sample_rate);
        assert(current_track->metadata.num_channels == num_channels);
        assert(current_track->metadata.num_values == num_values);
    }

    // add all track data together and load into new mixed track:
    for (int i = 0; i < num_values; i++) {
        for (int j = 0; j < num_tracks; j++) {
            track current_track = tracks[j];
            mixed_track_data[i] += current_track->data[i];
        }
    }

    mixed_track->playing = 1;
    mixed_track->muted = 0;
    mixed_track->data = mixed_track_data;

    struct metadata new_metadata = {tracks[0]->metadata.bpm,
                                    NA,
                                    sample_rate,
                                    1,
                                    num_channels,
                                    FLOAT_IND,
                                    num_values,
                                    num_values / num_channels};
    mixed_track->metadata = new_metadata;

    normalise_track(mixed_track);

    return mixed_track;
}

void destroy_track(track t) {
    f_printf("destroy_track: Freeing track data\n");
    f_printf("destroy_track: Track data is %snull\n", t->data == NULL ? "" : "not ");
    free(t->data);
    f_printf("destroy_track: Freeing `struct track`\n");
    free(t);
}
