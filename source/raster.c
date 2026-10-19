#include "raster.h"
static int clamp(int x,int limit){return x<0?0:x>limit?limit:x;}
void raster_init(RasterView *v,uint16_t *pixels,const ArenaCamera *camera){
    v->pixels=pixels;v->camera=*camera;v->scene=0;v->occlusion=0;v->scene_palette=0;v->layer_depth=0;v->scene_shift=0;v->background_sample=0;v->foreground_reject=0;
    for(int x=0;x<256;x++)v->x[x]=camera->x+x*camera->width/256;
    int square_pixels=camera->height*4==camera->width*3;
    for(int y=0;y<192;y++){v->y[y]=camera->y+(square_pixels?y*camera->width/256:y*camera->height/192);v->dirty_left[y]=256;v->dirty_right[y]=0;}
}
static void bounds(const RasterView *v,int x,int y,int w,int h,int *left,int *top,int *right,int *bottom){
    /* Conservative bounds; world-coordinate checks below preserve exact sampling. */
    *left=clamp((x-v->camera.x)*256/v->camera.width-1,256);
    *right=clamp((x+w-v->camera.x)*256/v->camera.width+2,256);
    *top=clamp((y-v->camera.y)*192/v->camera.height-1,192);
    *bottom=clamp((y+h-v->camera.y)*192/v->camera.height+2,192);
}
void raster_sprite(RasterView *v,const uint8_t *source,const uint16_t *palette,
                   int x,int y,int width,int height,int flip,int opacity,int drawn_height,int depth){
    int left,top,right,bottom;
    bounds(v,x,y,width,drawn_height,&left,&top,&right,&bottom);
    for(int yy=top;yy<bottom;yy++){
        int sy=v->y[yy]-y;if(sy<0||sy>=drawn_height)continue;
        if(drawn_height!=height)sy=sy*height/drawn_height;
        const uint8_t *row=source+sy*width;
        uint16_t *out=v->pixels+yy*256;
        if(left<v->dirty_left[yy])v->dirty_left[yy]=left;
        if(right>v->dirty_right[yy])v->dirty_right[yy]=right;
        for(int xx=left;xx<right;xx++){
            int sx=v->x[xx]-x;if(sx<0||sx>=width)continue;
            unsigned index=row[flip?width-1-sx:sx];if(!index)continue;
            if(v->foreground_reject&&v->foreground_reject(v->x[xx],v->y[yy]))continue;
            int world_index=v->y[yy]*WORLD_WIDTH+v->x[xx];
            if(v->occlusion&&depth<32767){
                unsigned mask=v->occlusion[world_index];
                if(mask&&v->layer_depth[mask-1]>depth)continue;
            }
            uint16_t color=palette[index];
            if(opacity==16){out[xx]=color;continue;}
            uint16_t back=out[xx];if(!(back&0x8000)&&v->background_sample)back=v->background_sample(v->x[xx],v->y[yy]);
            else if(!(back&0x8000)&&v->scene)back=v->scene_palette[v->scene[v->scene_shift?(v->y[yy]>>1)*(WORLD_WIDTH/2)+(v->x[xx]>>1):world_index]];
            int b=16-opacity;
            unsigned r=((color&31)*opacity+(back&31)*b)>>4;
            unsigned g=(((color>>5)&31)*opacity+((back>>5)&31)*b)>>4;
            unsigned blue=(((color>>10)&31)*opacity+((back>>10)&31)*b)>>4;
            out[xx]=0x8000|r|(g<<5)|(blue<<10);
        }
    }
}
/* Foreground already exists in terrain. Restore only pixels that actors or
   effects may cover; widening damage here caused neighboring wall strips to
   redraw one another all the way across the screen before the shop. */
void raster_foreground(RasterView *v,const uint8_t *source,const uint16_t *palette,int x,int y,int width,int height){
    for(int yy=0;yy<192;yy++){
        int sy=v->y[yy]-y;if(sy<0||sy>=height)continue;
        const uint8_t *row=source+sy*width;uint16_t *out=v->pixels+yy*256;
        for(int xx=v->dirty_left[yy];xx<v->dirty_right[yy];xx++){
            int sx=v->x[xx]-x;if(sx<0||sx>=width)continue;
            unsigned c=row[sx];if(c)out[xx]=palette[c];
        }
    }
}
void raster_rect(RasterView *v,int x,int y,int width,int height,uint16_t color,int depth){
    int left,top,right,bottom;bounds(v,x,y,width,height,&left,&top,&right,&bottom);
    for(int yy=top;yy<bottom;yy++)if(v->y[yy]>=y&&v->y[yy]<y+height){
        if(left<v->dirty_left[yy])v->dirty_left[yy]=left;
        if(right>v->dirty_right[yy])v->dirty_right[yy]=right;
        for(int xx=left;xx<right;xx++)if(v->x[xx]>=x&&v->x[xx]<x+width){
            if(v->foreground_reject&&v->foreground_reject(v->x[xx],v->y[yy]))continue;
            unsigned mask=v->occlusion?v->occlusion[v->y[yy]*WORLD_WIDTH+v->x[xx]]:0;
            if(!mask||v->layer_depth[mask-1]<=depth)v->pixels[yy*256+xx]=color;
        }
    }
}
/* Soft particles only; keep the common opaque sprite path free of alpha work. */
void raster_sprite_alpha(RasterView *v,const uint8_t *source,const uint8_t *alpha,const uint16_t *palette,
                         int x,int y,int width,int height,int flip,int depth){
    int left,top,right,bottom;bounds(v,x,y,width,height,&left,&top,&right,&bottom);
    for(int yy=top;yy<bottom;yy++){
        int sy=v->y[yy]-y;if(sy<0||sy>=height)continue;
        if(left<v->dirty_left[yy])v->dirty_left[yy]=left;
        if(right>v->dirty_right[yy])v->dirty_right[yy]=right;
        int world_row=v->y[yy]*WORLD_WIDTH;
        for(int xx=left;xx<right;xx++){
            int sx=v->x[xx]-x;if(sx<0||sx>=width)continue;
            int at=sy*width+(flip?width-1-sx:sx),a=alpha[at];
            if(!a||!source[at])continue;
            if(v->foreground_reject&&v->foreground_reject(v->x[xx],v->y[yy]))continue;
            int world=world_row+v->x[xx];
            unsigned mask=v->occlusion?v->occlusion[world]:0;
            if(mask&&v->layer_depth[mask-1]>depth)continue;
            uint16_t color=palette[source[at]],back=v->pixels[yy*256+xx];
            if(!(back&0x8000)&&v->background_sample)back=v->background_sample(v->x[xx],v->y[yy]);
            else if(!(back&0x8000)&&v->scene)back=v->scene_palette[v->scene[v->scene_shift?(v->y[yy]>>1)*(WORLD_WIDTH/2)+(v->x[xx]>>1):world]];
            int b=16-a;
            unsigned red=((color&31)*a+(back&31)*b)>>4;
            unsigned green=(((color>>5)&31)*a+((back>>5)&31)*b)>>4;
            unsigned blue=(((color>>10)&31)*a+((back>>10)&31)*b)>>4;
            v->pixels[yy*256+xx]=0x8000|red|(green<<5)|(blue<<10);
        }
    }
}
