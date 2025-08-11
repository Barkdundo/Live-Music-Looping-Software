#include "SoundTouchDLL.h"

extern "C" {
void *st_new() { return soundtouch_createInstance(); }
void st_free(void *h) { soundtouch_destroyInstance(h); }

void st_set_sr(void *h, unsigned sr) { soundtouch_setSampleRate(h, sr); }
void st_set_ch(void *h, unsigned ch) { soundtouch_setChannels(h, ch); }
void st_set_tempo(void *h, float t) { soundtouch_setTempo(h, t); }
void st_set_pitch(void *h, float st) { soundtouch_setPitchSemiTones(h, st); }

unsigned st_put(void *h, const float *src, unsigned n) {
    return soundtouch_putSamples(h, src, n);
}

unsigned st_recv(void *h, float *dst, unsigned n) {
    return soundtouch_receiveSamples(h, dst, n);
}

void st_flush(void *h) { soundtouch_flush(h); }
}