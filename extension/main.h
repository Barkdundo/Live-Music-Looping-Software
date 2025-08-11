#pragma once

#include "types.h"

#ifdef TEST_BUILD
#include "constants.h"
int get_filenames(char[][MAX_FILENAME_LEN]);
int pair_files(int, char[][MAX_FILENAME_LEN], music_pair *);
input_data generate_input_data();
#endif

#ifndef TEST_BUILD
extern volatile state s;
#endif
