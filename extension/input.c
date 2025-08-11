#include "input.h"
#include "queue.h"
#include "types.h"
#include <ctype.h>
#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef TEST_BUILD
#include "main.h"
#else
#include "test/test.h"
#endif
#include "constants.h"

static void f_printf(const char *format, ...) {
    FILE *file = fopen("debuginput.log", "a");

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

// COMMANDS:
// Quit: /
// Select Single Track: 0 - 9 numkeys
// Change Selection mode: #
// Clear Selections: `
// Start loop recording: , --not implemennted beyond here
// Volume down: -
// Volume up: =
// Mute: m
// Play/Pause: p
// Remove Track: v
// Clear Command Input: q
// Load Track: lx  (space at end) where x = track_id
// Change BPM: bx  (space at end) where x = target BPM

int char_comp(const void *a, const void *b) {
    return (*(char *)a - *(char *)b);
}

static char single_char_comms[] = "#,-./0123456789=`mpqv";
static int def_reg_array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static char play_mode_comms[] = {",acefghjkstuwy"};
static int current_selection = -1;
static int curr_selection_array[10] = {};
static int selection_mode = 0;
static int no_selected = 0;
static bool loop_rec = false;
static int command_mode = 0;
static bool quit = false;

bool check_command(char *);

struct command error_command = {-1, -1, -1, -1};

bool is_a_single_char_comm(char c) {
    for (int i = 0; i < 21; i++) {
        if (single_char_comms[i] == c) {
            return true;
        }
    }
    return false;
}

bool is_a_play_mode_val_chars(char c) {
    return bsearch(&c, play_mode_comms, 14, 1, char_comp) != NULL;
}

bool check_compound_command(char *input) {
    int len = strlen(input);
    if (len < 2) {
        return false;
    }
    int end_of_command = 0;
    for (int i = len - 2; i > 0; i--) {
        if (!isdigit(input[i])) {
            end_of_command = i;
            break;
        }
    }
    char temp_string[50] = "";
    for (int i = 1; i <= end_of_command; i++) {
        char temp_concat_str[2] = {input[i], '\0'};
        strcat(temp_string, temp_concat_str);
    }
    if (check_command(temp_string)) {
        return true;
    }

    return false;
}

int calculate_alignment(int reg_num) {
    if (s.registers[reg_num] == NULL) {
        return -1;
    }
    return (((s.tick_counter + 2) / (BEATS_PER_BAR * 4)) + 1) * (BEATS_PER_BAR * 4);
}

struct command build_simple_command(command_type c_type, int reg_num,
                                    int payload) {
    int alignment;
    int temp_calc = calculate_alignment(reg_num);
    if (temp_calc == -1) {
        exit(1);
    }
    // in executing, we look at all commands that have tick < current counter,
    // so if it's below we will still execute it
    alignment = s.tick_counter;
    if (c_type == PLAY) {
        alignment = calculate_alignment(reg_num);
        payload = 1;
    }

    struct command ret_command = {reg_num, alignment, c_type, payload};
    return ret_command;
}

void simple_command_enqueue(command_type c_type, int payload) {
    if (selection_mode == 0 && current_selection != -1) {
        if (s.registers[current_selection] == NULL) {
        } else {
            struct command c =
                build_simple_command(c_type, current_selection, payload);
            pq_enqueue(s.queue, c);
        }

    } else {
        int loop_lim = 0;
        int *num_pointer;
        no_selected
            ? (loop_lim = no_selected, num_pointer = curr_selection_array)
            : (loop_lim = 10, num_pointer = def_reg_array);
        for (int i = 0; i < loop_lim; i++) {
            if (s.registers[num_pointer[i]] != NULL) {
                struct command c =
                    build_simple_command(c_type, num_pointer[i], payload);
                pq_enqueue(s.queue, c);
            }
        }
    }
}

bool check_command(char *input) {
    int len = strlen(input);
    if (len <= 0) {
        return false;
    }
    int end_of_string = len - 1;
    if (len > 2 && input[0] == 'l' && input[end_of_string] == ' ') {
        for (int i = 1; i < end_of_string; i++) {
            if (!isdigit(input[i])) {
                return false;
            }
        }
        return true;
    } else if (len == 1 && is_a_single_char_comm(input[0])) {
        return true;
    } else if (command_mode == 1 && len == 1 &&
               is_a_play_mode_val_chars(input[0])) {
        return true;
    } else if (len > 2 && input[0] == 'b' && input[end_of_string] == ' ') {
        for (int i = 1; i < end_of_string; i++) {
            if (!isdigit(input[i])) {
                return false;
            }
        }
        return true;
    } else if (len > 0 && input[len - 1] == 'q') {
        return true;
    }

    return false;
}

void parse_command(char *input) {
    int len = strlen(input);
    if (len <= 0) {
        return;
    }
    int end_of_string = len - 1;
    if (command_mode == 0) {
        if (isdigit(input[0])) {
            int digit = atoi(input);
            if (selection_mode == 0 && current_selection != digit) {
                current_selection = digit;
            } else if (selection_mode == 0) {
                current_selection = -1;
            } else if (selection_mode == 1) {
                int old_selected = no_selected;
                for (int i = 0; i < no_selected; i++) {
                    if (curr_selection_array[i] == digit) {
                        for (int j = i; j < no_selected - 1; j++) {
                            curr_selection_array[j] =
                                curr_selection_array[j + 1];
                        }
                        no_selected--;
                    }
                }
                if (old_selected == no_selected) {
                    curr_selection_array[no_selected] = digit;
                    no_selected++;
                }
            }
        } else if (input[0] == '#') {
            selection_mode ? selection_mode = 0, current_selection = -1 : (selection_mode = 1);
            no_selected = 0;

        } else if (input[0] == ',') {
            loop_rec ? loop_rec = false : (loop_rec = true);
        } else if (input[0] == 'm') {
            simple_command_enqueue(MUTE, 0);
        } else if (input[0] == '`') {
            no_selected = 0;
            current_selection = -1;
        } else if (input[0] == 'p') {
            command_type c_type = PLAY;
            if ((selection_mode == 0 && current_selection == -1) ||
                (selection_mode == 1 && no_selected <= 0)) {
                for (int i = 0; i < 10; i++) {
                    if (s.registers[i] == NULL) {
                        break;
                    }
                    if (s.registers[i]->playing) {
                        c_type = PAUSE;
                    }
                }
                simple_command_enqueue(c_type, 0);
            }
            if (selection_mode == 0 && current_selection != -1) {
                if (s.registers[current_selection] != NULL &&
                    s.registers[current_selection]->playing) {
                    c_type = PAUSE;
                }

                simple_command_enqueue(c_type, 0);
            } else {
                for (int i = 0; i < no_selected; i++) {
                    if (s.registers[i] == NULL) {
                        break;
                    }
                    if (s.registers[curr_selection_array[i]] != NULL &&
                        s.registers[curr_selection_array[i]]->playing) {
                        c_type = PAUSE;
                    }
                }
                simple_command_enqueue(c_type, 0);
            }
        } else if (input[0] == 'v') {
            simple_command_enqueue(REMOVE, 0);
        } else if (input[0] == 'c') {
            command_mode = 1;
        } else if (input[0] == '-') {
            simple_command_enqueue(VOLUME_DOWN, 0);
        } else if (input[0] == '=') {
            simple_command_enqueue(VOLUME_UP, 0);
        } else if (input[0] == '/') {
            quit = true;
        } else if (input[0] == 'l') {
            int track_id = atoi(&input[1]);
            command_type c_type = LOAD;
            int alignment;
            if (s.tracks[track_id] != NULL) {
                alignment = s.tick_counter;
                struct command comm = {current_selection, alignment, c_type,
                                       track_id};
                pq_enqueue(s.queue, comm);
            }

        }

        else if (input[0] == 'b') {
            char target_BPM_s[10] = "";
            int target_BPM;
            for (int i = 1; i < end_of_string - 1; i++) {
                char temp_concat_str[2] = {input[i], '\0'};
                strcat(target_BPM_s, temp_concat_str);
            }
            target_BPM = atoi(target_BPM_s);

            simple_command_enqueue(BPM_SET, target_BPM);

        } else if (command_mode == 0 && input[0] == 'f') {
            // fades
        } else if (command_mode == 0 && input[0] == 'd') {

        } else if (command_mode == 1) {
            if (input[0] == 'c') {
                command_mode = 0;
            } else {
                // play note
            }
        } else if (len > 0 && input[end_of_string] == 'q') {
            strcpy(input, "");
        }
    }
}

char *keys[] = {"C",         "C_SHARP", "D",         "D_SHARP",   "E",
                "F",         "F_SHARP", "G",         "G_SHARP",   "A",
                "A_SHARP",   "B",       "C_M",       "C_SHARP_M", "D_M",
                "D_SHARP_M", "E_M",     "F_M",       "F_SHARP_M", "G_M",
                "G_SHARP_M", "A_M",     "A_SHARP_M", "B_M",       "NA"};

void print_tracks(char *input) {

    move(0, 0);
    for (int i = 0; i < 20; i++) {
        if (i % 3 == 0 && i != 0) {
            int x, y;
            getyx(stdscr, y, x);
            move(y + 1, x - x);
        }
        if (s.tracks[i] == NULL || s.tracks[i] == 0) {
            printw("Track %d not loaded", i);
        } else {
            printw("Track %d: name :%s  ", i, s.tracks[i]->name);
        }
        int x, y;
        getyx(stdscr, y, x);
        move(y, x + 1);
    }

    move(10, 0);
    start_color();

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    bool change_colour = false;

    for (int i = 0; i < 10; i++) {

        if (selection_mode == 0 && i == current_selection) {
            change_colour = true;
            attron(COLOR_PAIR(1));
        } else {
            for (int j = 0; j < no_selected; j++) {
                if (i == curr_selection_array[j]) {
                    attron(COLOR_PAIR(1));
                    change_colour = true;
                }
            }
        }
        if (i % 3 == 0 && i != 0) {
            int x, y;
            getyx(stdscr, y, x);
            move(y + 1, x - x);
        }
        if (s.registers[i] == NULL) {

            printw("Register %d: not loaded", i);
            clrtoeol();
        } else {
            int x, y;
            getyx(stdscr, y, x);
            move(y, x);
            printw("Register %d: name %s", i, s.registers[i]->name);
            move(y, 38);
            printw(" BPM: %d Key: %s", s.registers[i]->metadata.bpm,
                   keys[s.registers[i]->metadata.key]);
        }
        int a, b;

        getyx(stdscr, b, a);
        move(b + 1, a - a);
        if (change_colour) {
            attroff(COLOR_PAIR(1));
        }
    }

    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    attron(COLOR_PAIR(2));
    move(LINES -2, 0);
    int current_bar = s.tick_counter / BEATS_PER_BAR;
    int current_beat = s.tick_counter % BEATS_PER_BAR + 1; 
    int current_target = (((s.tick_counter + 2) / (BEATS_PER_BAR * 4)) + 1) * (BEATS_PER_BAR);
    printw("Current bar: %d  Current beat in bar: %d Current Target: %d", current_bar, current_beat, current_target);
    attroff(COLOR_PAIR(1));
    move(LINES - 1, 0);
    if (selection_mode == 0) {
        printw("Current Selection: %d Input: %s", current_selection, input);
    } else {
        printw("Current Selection: ");
        for (int i = 0; i < no_selected; i++) {
            printw("%d ", curr_selection_array[i]);
        }
        printw("Input: %s", input);
    }
    clrtoeol();
    refresh();
}

void *run_input(void *_) {

    char input_char;
    char input_string[100] = "";
    bool is_valid_command = false;
    print_tracks("NA");

    echo();
    do {
        do {
            timeout(50);
            print_tracks(input_string);
            refresh();
            input_char = getch();
            if (input_char != ERR) {
                char temp_str[] = {input_char, '\0'};
                strcat(input_string, temp_str);
                is_valid_command = check_command(input_string);
            }

        } while (!is_valid_command);
        // execute the corresponding command
        parse_command(input_string);
        strcpy(input_string, "");

    } while (!quit);
    clear();
    refresh();
    endwin();
    return NULL;
}
