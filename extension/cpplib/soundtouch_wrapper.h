#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *st_new(void);
void st_free(void *);

void st_set_sr(void *, unsigned);
void st_set_ch(void *, unsigned);
void st_set_tempo(void *, float);
void st_set_pitch(void *, float);

unsigned st_put(void *, const float *, unsigned);
unsigned st_recv(void *, float *, unsigned);
void st_flush(void *);

#ifdef __cplusplus
}
#endif
