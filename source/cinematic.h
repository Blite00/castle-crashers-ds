#ifndef CINEMATIC_H
#define CINEMATIC_H
#include <stdint.h>
enum { CINEMATIC_CRYSTAL=0, CINEMATIC_PRINCESSES=1 };
/* Original30-Hz source timing; movie frames sample at15fps. Last image holds. */
unsigned cinematic_duration(unsigned kind);
/* Stable clip-local image key for caller's rendering cache. Random seek safe. */
unsigned cinematic_frame(unsigned kind,unsigned tick);
/* Source world viewport origin (424x318) for seamless game-camera handoff. */
void cinematic_camera(unsigned kind,unsigned tick,int *x,int *y);
/* Optional borrowed workspace: caller must reset before reusing its bytes.
 *128000bytes caches both throne actor blocks/background, or the larger
 * princess background with framebuffer actor decoding. NULL keeps the
 * independent framebuffer path; no extra persistent pixel allocation. */
void cinematic_set_workspace(void *memory,unsigned bytes);
void cinematic_reset(void);
unsigned cinematic_inflate_count(void);
/* Fill256x192 RGB555; sequential and arbitrary seeks remain bit-identical. */
void cinematic_draw(uint16_t *pixels,unsigned kind,unsigned tick);
#endif
