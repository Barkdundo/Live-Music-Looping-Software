#include "loop.h"

#include <stdarg.h>
#include <time.h>
#include <windows.h>

#include "audio_writer.h"
#include "constants.h"
#include "main.h"
#include "queue.h"
#include "track.h"

static void f_printf(const char *format, ...) {
    FILE *file = fopen("debugloop.log", "a");

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

static void cmd_load(struct command cmd) {
    // move the track from the normal tracks to the specified register
    s.registers[cmd.track] = s.tracks[cmd.payload];
}

static void cmd_remove(struct command cmd) {
    // stops the playback of a track in a register
    s.registers[cmd.track]->muted = 1;
    s.registers[cmd.track] = NULL;
}

static void cmd_pause(struct command cmd) {
    // call the track's pause function (not decided yet)
    s.registers[cmd.track]->muted = 1;
    s.registers[cmd.track]->playing = 0;
}

static void cmd_mute(struct command cmd) {
    // call the track's mute function
    mute(s.registers[cmd.track]);
}

static void cmd_volume_up(struct command cmd) {
    // call the track's volume up (not decided yet)
    volume_up(s.registers[cmd.track]);
}

static void cmd_volume_down(struct command cmd) {
    // call the track's volume down (not decided yet)
    volume_down(s.registers[cmd.track]);
}

static void cmd_bpm_set(struct command cmd) {
    // future feature, if you can mix effectively
    // while changing bpm good for you
}

typedef void (*command_handler)(struct command);

static command_handler command_handlers[8] = {
    cmd_load, cmd_remove,    NULL /* PLAY */, cmd_pause,
    cmd_mute, cmd_volume_up, cmd_volume_down, cmd_bpm_set};

static HANDLE timer;

void win_sleep(long long nanoseconds) {
    LARGE_INTEGER due;
    // we negate to delay relatively, and we divide by 100
    // for the resolution of the timer
    due.QuadPart = -(nanoseconds / 100);

    // we don't need the extended API here
    SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE);
    WaitForSingleObject(timer, INFINITE);
}

void *run_loop(void *_) {
    // initialise the timer for the loop

    // we create with the extended windows API for high resolution
    timer = CreateWaitableTimerEx(NULL, NULL,
                                  CREATE_WAITABLE_TIMER_HIGH_RESOLUTION |
                                      CREATE_WAITABLE_TIMER_MANUAL_RESET,
                                  TIMER_ALL_ACCESS);

    while (1) {
        // inspect commands

        f_printf("run_loop: Tick\n");
        struct command to_execute[MAX_QUEUE_LENGTH];
        int num_commands_to_execute = 0;

        // while peeking gives us a command which has target tick less than or
        // equal to our current tick, pop it out, and add it to `to_execute`

        while (s.queue->size > 0 &&
               (pq_peek(s.queue).tick - 1) <= s.tick_counter) {
            to_execute[num_commands_to_execute++] = pq_dequeue(s.queue);
            f_printf("run_loop: Popping command out of PQ\n");
        }

        // handle each command

        track to_play[MAX_LOADED_TRACKS];
        int num_tracks = 0;

        for (int i = 0; i < num_commands_to_execute; i++) {
            struct command current = to_execute[i];

            if (current.type == PLAY) {
                f_printf("run_loop: Handling PLAY command\n");
                // if the track isn't muted, then we can play it (only field for
                // now) set the playing field accordingly
                if (s.registers[current.track] == NULL) {
                    continue;
                }
                track current_track = s.registers[current.track];
                current_track->playing = !current_track->muted;
                if (current.payload) {
                    current_track->playing = 1;
                    current_track->muted = 0;
                }
                if (!current_track->muted) {
                    // the track is to be played, now which time is it?
                    if (current.payload == 1) {
                        // starting offset is the target tick
                        current_track->start_offset = current.tick;
                    }
                    to_play[num_tracks++] = s.registers[current.track];

                    // since we have the register its targeting currently,
                    // reschedule the play command again here

                    struct command new = {
                        .track = current.track,
                        .tick = current.tick + BEATS_PER_BAR,
                        .type = PLAY,
                        .payload = 0,
                    };

                    pq_enqueue(s.queue, new);
                }
            } else {
                f_printf("run_loop: Handling other command\n");
                command_handlers[current.type](current);
            }
        }

        if (num_tracks > 0) {
            f_printf("run_loop: More than 1 track to be played\n");
            // compute the current bar to be played
            for (int i = 0; i < num_tracks; i++) {
                // compute track bar
                track current_track = to_play[i];
                int loop_length = current_track->metadata.loop_length;

                // we use s.tick_counter + 1 because we're processing this
                // a tick early
                int current_bar =
                    (((s.tick_counter + 1) - current_track->start_offset) /
                        BEATS_PER_BAR) %
                    loop_length;
                current_track = get_bar(current_track, current_bar);

                if (current_track == NULL) {
                    f_printf("run_loop: get_bar returned NULL pointer\n");
                }

                to_play[i] = current_track;
            }

            // mix them all together
            track combined = mix_tracks(to_play, num_tracks);

            f_printf("run_loop: Tracks mixed, about to play\n");

            // play the combined track
            audio_writer(combined, s.input_data);

            f_printf("run_loop: Tracks written to buffer\n");

            // free the combined track
            destroy_track(combined);

            f_printf("run_loop: Mixed track freed\n");

            // free the bars
            for (int i = 0; i < num_tracks; i++) {
                f_printf("run_loop: Freeing bar %d\n", i);
                destroy_track(to_play[i]);
                f_printf("run_loop: Freed bar %d\n", i);
            }

            f_printf("run_loop: Bars freed\n");
        } else if ((s.tick_counter + 1) % 16 == 0) {
            audio_clear(s.input_data);
        }

        // to increment after processing is done for our tick

        f_printf("run_loop: Sleeping until next tick\n");

        int64_t to_sleep = SECONDS_PER_MINUTE * NANOSECONDS_PER_SECOND / s.bpm;

        win_sleep(to_sleep);
        s.tick_counter++;
    }
}
