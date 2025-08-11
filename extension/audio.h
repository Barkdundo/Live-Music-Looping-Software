#pragma once

#include <portaudio.h>

#include "types.h"

extern int play_audio(input_data, PaStream **);

extern int end_audio(PaStream *);
