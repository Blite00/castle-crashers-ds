#ifndef RASTER_H
#define RASTER_H
#include "camera.h"
#if defined(ARM9) && defined(__arm__)
/* Keep the pixel loop in ARM9 instruction TCM. ARM mode has more registers
   for sampling/blending than Thumb, reducing spills in this hot path. */
#define RASTER_FAST __attribute__((target("arm"),section(".itcm.text.raster_sprite"),long_call))
#else
#define RASTER_FAST
#endif
typedef struct {
    uint16_t *pixels;
    ArenaCamera camera;
    int x[256],y[192];
    uint16_t dirty_left[192],dirty_right[192];
    const uint8_t *scene,*occlusion;
    const uint16_t *scene_palette;
    const int16_t *layer_depth;
    unsigned scene_shift;
    uint16_t (*background_sample)(int x,int y);
    int (*foreground_reject)(int x,int y);
} RasterView;
void raster_init(RasterView *view,uint16_t *pixels,const ArenaCamera *camera);
RASTER_FAST void raster_sprite(RasterView *view,const uint8_t *source,const uint16_t *palette,
                   int x,int y,int width,int height,int flip,int opacity,int drawn_height,int depth);
void raster_foreground(RasterView *view,const uint8_t *source,const uint16_t *palette,int x,int y,int width,int height);
void raster_rect(RasterView *view,int x,int y,int width,int height,uint16_t color,int depth);
void raster_sprite_alpha(RasterView *view,const uint8_t *source,const uint8_t *alpha,const uint16_t *palette,
                         int x,int y,int width,int height,int flip,int depth);
#endif
