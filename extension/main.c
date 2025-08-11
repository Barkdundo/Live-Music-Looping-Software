#include <assert.h>
#include <curses.h>
#include <dirent.h>
#include <portaudio.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// ignore test
#include "audio.h"
#include "constants.h"
#include "handlers/wav.h"
#include "input.h"
#include "loop.h"
#include "queue.h"
#include "track.h"
#include "types.h"

#ifndef TEST_BUILD
volatile state s = {
    .tick_counter = 0,
    .bpm = DEFAULT_BPM,
};
#endif

static void f_printf(const char *format, ...) {
    FILE *file = fopen("debugmain.log", "a");

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

int get_filenames(char files[][MAX_FILENAME_LEN]) {
    DIR *directory_handle;
    struct dirent *entry;

    directory_handle = opendir(SOUND_DIR);
    if (!directory_handle) {
        // fatal error
    }

    int file_count = 0;
    while ((entry = readdir(directory_handle)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        strncpy(files[file_count++], entry->d_name, MAX_FILENAME_LEN - 1);
    }

    closedir(directory_handle);

    f_printf("get_filenames: Filenames found\n");

    return file_count;
}

int has_extension(char *filename, char *ext) {
    char *extension = strrchr(filename, '.');
    return (extension != NULL) && (strcmp(extension, ext) == 0);
}

char *get_filename_without_extension(char *filename) {
    char *new = strdup(filename);
    char *last_dot = strrchr(new, '.');

    if (last_dot != NULL) {
        *last_dot = '\0';
    }

    return new;
}

int pair_files(int file_count, char filenames[][MAX_FILENAME_LEN],
               music_pair *pairs) {
    int pair_count = 0;

    for (int i = 0; i < file_count; i++) {
        char *filename = filenames[i];
        if (!(has_extension(filename, METADATA_EXTENSION))) {
            continue;
        }

        // otherwise, if it's a .dat file, find the corresponding
        // audio file that matches

        char *current_file = get_filename_without_extension(filename);

        for (int j = 0; j < file_count; j++) {
            // don't match against self
            if (i == j)
                continue;

            char *comparison_file =
                get_filename_without_extension(filenames[j]);

            if (strcmp(current_file, comparison_file) == 0) {
                // found match

                char data_name_buffer[MAX_FILENAME_LEN + strlen(SOUND_DIR) + 1];
                char metadata_name_buffer[MAX_FILENAME_LEN + strlen(SOUND_DIR) +
                                          1];

                snprintf(data_name_buffer, sizeof(data_name_buffer), "%s%s",
                         SOUND_DIR, filenames[j]);
                snprintf(metadata_name_buffer, sizeof(metadata_name_buffer),
                         "%s%s", SOUND_DIR, filename);

                pairs[pair_count].data = fopen(data_name_buffer, "rb");
                pairs[pair_count].metadata = fopen(metadata_name_buffer, "r");
                strncpy(pairs[pair_count].name, comparison_file,
                        MAX_FILENAME_LEN - 1);
                pair_count++;
            }

            free(comparison_file);
        }

        free(current_file);
    }

    f_printf("pair_files: Files paired\n");

    return pair_count;
}

input_data generate_input_data() {
    // required continually for PortAudio callback
    float *input_buffer = malloc(BUFFER_SIZE * sizeof(float));
    assert(input_buffer != NULL);
    // required continually also for PortAudio callback
    input_data data = malloc(sizeof(struct input_data));
    assert(data != NULL);
    data->buffer_size = BUFFER_SIZE;
    data->input_buffer = input_buffer;
    data->read_index = 0;  // where to start inserting into output buffer
    data->write_index = 0; // where to start writing into input buffer
    assert(data->input_buffer != NULL);
    return data;
}

#ifndef TEST_BUILD
int main(int argc, char **argv) {
    // clear log file
    FILE *logmain = fopen("debugmain.log", "w");
    fclose(logmain);
    FILE *logloop = fopen("debugloop.log", "w");
    fclose(logloop);
    FILE *logtrack = fopen("debugtrack.log", "w");
    fclose(logtrack);

    char filenames[MAX_LOADED_TRACKS * 2][MAX_FILENAME_LEN];
    int num_files = get_filenames(filenames);

    music_pair pairs[MAX_LOADED_TRACKS];
    int num_pairs = pair_files(num_files, filenames, pairs);

    // create state

    // all required for program execution
    s.tracks = calloc(MAX_LOADED_TRACKS, sizeof(track));
    s.registers = calloc(NUM_REGISTERS, sizeof(track));
    s.queue = pq_create(MAX_QUEUE_LENGTH);

    f_printf("main: Created state\n");

    f_printf("num_pairs = %d", num_pairs);

    for (int i = 0; i < num_pairs; i++) {
        f_printf("%d\n",i);
        music_pair current_pair = pairs[i];

        s.tracks[i] = read_parse_wav(current_pair.data, current_pair.metadata);
        strncpy(s.tracks[i]->name, current_pair.name, MAX_FILENAME_LEN - 1);
        s.tracks[i]->playing = 0;
        s.tracks[i]->start_offset = 0;
        s.tracks[i]->muted = 0;

        if (s.tracks[i]->metadata.bpm != DEFAULT_BPM) {
            f_printf("reached if statement\n");
            set_bpm(s.tracks[i], DEFAULT_BPM);
        }
    }

    f_printf("main: Tracks created\n");

    // initialise curses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // initialise portaudio

    input_data data = generate_input_data();
    s.input_data = data;

    PaStream *stream;
    int err = play_audio(data, &stream);
    if (err) {
        printf("Audio initialisation failed\n");
        return 1;
    }

    // get all tracks

    pthread_t input_loop;
    pthread_t event_loop;

    pthread_create(&input_loop, NULL, run_input, NULL);
    pthread_create(&event_loop, NULL, run_loop, NULL);

    f_printf("main: Threads created\n");

    pthread_join(input_loop, NULL);
    pthread_join(event_loop, NULL);

    // close portaudio and free what it used

    end_audio(stream);

    free(data->input_buffer);
    free(data);

    return 0;
}
#endif
