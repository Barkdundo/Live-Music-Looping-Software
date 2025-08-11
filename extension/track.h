#pragma once

#include "types.h"

extern void set_bpm(track, int);
extern void volume_up(track);
extern void volume_down(track);
extern void mute(track);
extern void key_change(track, key);
extern track get_bar(track, int);
extern track mix_tracks(track *, int);
extern void normalise_track(track);
extern void destroy_track(track);