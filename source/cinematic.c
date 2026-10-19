#include "cinematic.h"
#include "cinematic_art.h"
#include "asset_inflate.h"
#include <string.h>

static struct {uint8_t *memory;unsigned bytes,kind,block,frame,patch,bg_ready,cx,cy,crop,layout;} cache;
static unsigned inflate_calls;
void cinematic_reset(void){cache.kind=cache.block=cache.frame=~0u;cache.bg_ready=0;cache.crop=~0u;}
void cinematic_set_workspace(void *memory,unsigned bytes){
    if(cache.memory!=(uint8_t *)memory||cache.bytes!=bytes){cache.memory=memory;cache.bytes=bytes;cinematic_reset();}
}
unsigned cinematic_inflate_count(void){return inflate_calls;}
static int inflate_counted(const uint8_t *in,unsigned size,uint8_t *out,unsigned bytes){inflate_calls++;return asset_inflate(in,size,out,bytes);}

unsigned cinematic_duration(unsigned kind){
    return kind==CINEMATIC_PRINCESSES?232u:364u;
}
unsigned cinematic_frame(unsigned kind,unsigned tick){
    unsigned count=kind==CINEMATIC_PRINCESSES?116u:182u,frame=tick/2u;
    return frame<count?frame:count-1;
}
void cinematic_camera(unsigned kind,unsigned tick,int *x,int *y){
    if(kind>CINEMATIC_PRINCESSES)return;
    unsigned f=cinema_clip_offsets[kind]+cinematic_frame(kind,tick);
    int sx=(cinema_bg_origins[kind*2]+cinema_camera[f*2])*53;
    int sy=(cinema_bg_origins[kind*2+1]+cinema_camera[f*2+1])*53;
    if(x)*x=(sx+(sx<0?-12:12))/24+100;
    if(y)*y=(sy+(sy<0?-12:12))/24+405;
}
/* Bounded reciprocal arithmetic: Thumb1 otherwise calls software division
 * four times for each source pixel. Inputs here are at most773. */
static unsigned third(unsigned value){return (value*21846u)>>16;}
/* Exact floor(value/255) for our0..8032 color numerators. */
static unsigned div255(unsigned value){return (value+1u+(value>>8))>>8;}
static void put_pixel(uint16_t *pixels,unsigned sx,unsigned sy,uint16_t color){
    for(unsigned yy=third(sy*4+1);yy<third((sy+1)*4+1);yy++)
        for(unsigned xx=third(sx*4+1);xx<third((sx+1)*4+1);xx++)pixels[yy*256+xx]=color;
}
static void shift_indexed(uint8_t *p,int dx,int dy){
    if(!dx&&!dy)return;
    /* Visit destination rows in the order that leaves unread source intact. */
    int y=dy<0?143:0,step=dy<0?-1:1;
    for(int n=0;n<144;n++,y+=step){
        int sy=y+dy,left=dx<0?-dx:0,right=dx>0?192-dx:192;
        uint8_t *out=p+y*192;
        if(sy<0||sy>=144||left>=right){memset(out,0,192);continue;}
        memmove(out+left,p+sy*192+left+dx,(unsigned)(right-left));
        if(left)memset(out,0,(unsigned)left);
        if(right<192)memset(out+right,0,(unsigned)(192-right));
    }
}
static unsigned read16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
void cinematic_draw(uint16_t *pixels,unsigned kind,unsigned tick){
    if(!pixels||kind>CINEMATIC_PRINCESSES)return;
    unsigned local=cinematic_frame(kind,tick),base=cinema_clip_offsets[kind];
    unsigned frame=base+local,first=base+(local/CINEMA_KEY_INTERVAL)*CINEMA_KEY_INTERVAL;
    unsigned brightness=255;
    if(kind==CINEMATIC_CRYSTAL){
        /* Source camera392 starts the global fader atframe15. MAINsprite42
           eases index toward255; global FadeOut(30) later is AUDIO only. */
        brightness=0;unsigned source_frame=2+local*2;
        for(unsigned f=15;f<=source_frame&&brightness<255;f++){
            unsigned diff=(255-brightness)/7;
            brightness=diff>10?brightness+diff:255;
        }
        if(!brightness){for(unsigned i=0;i<256u*192u;i++)pixels[i]=0x8000;return;}
    }

    unsigned block=cinema_clip_blocks[kind]+local/CINEMA_KEY_INTERVAL;
    unsigned width=cinema_bg_widths[kind],cx=cinema_camera[frame*2],cy=cinema_camera[frame*2+1];
    unsigned max_block=0;
    for(unsigned i=cinema_clip_blocks[kind];i<cinema_clip_blocks[kind+1];i++)
        if(cinema_block_sizes[i]>max_block)max_block=cinema_block_sizes[i];
    /* Princess camera pans across a wide backdrop. Cache only the region used
       by one actor key block (half a block during the wide opening pan). */
    unsigned window=local<16?8:16,crop=local&~(window-1);
    unsigned rx=cx,ry=cy,right=cx+192,bottom=cy+144;
    if(kind==CINEMATIC_PRINCESSES){
        unsigned end=crop+window;if(end>116)end=116;
        for(unsigned i=crop;i<end;i++){
            unsigned x=cinema_camera[(base+i)*2],y=cinema_camera[(base+i)*2+1];
            if(x<rx)rx=x;
            if(y<ry)ry=y;
            if(x+192>right)right=x+192;
            if(y+144>bottom)bottom=y+144;
        }
    }
    unsigned crop_width=right-rx,crop_height=bottom-ry;
    int crop_cached=kind==CINEMATIC_PRINCESSES&&cache.memory&&
        cache.bytes>=cinema_block_sizes[block]+crop_width*crop_height;
    if(crop_cached)max_block=cinema_block_sizes[block];
    unsigned bg_bytes=(cinema_bg_clip_rows[kind+1]-cinema_bg_clip_rows[kind])*CINEMA_BG_ROWS*width;
    int full_bg=!crop_cached&&cache.memory&&cache.bytes>=bg_bytes;
    int cached=crop_cached||(cache.memory&&cache.bytes>=max_block+(full_bg?bg_bytes:192u*144u));
    int bg_cached=full_bg||cached;
    unsigned layout=crop_cached?1u:full_bg?2u:cached?3u:0u;
    if(cache.layout!=layout){cache.bg_ready=0;cache.block=~0u;cache.layout=layout;}
    uint8_t *indexed=cached?cache.memory:(uint8_t *)pixels;
    uint8_t *background=bg_cached?cache.memory+(cached?max_block:0):NULL;
    int rebuild=!cached||cache.kind!=kind||cache.block!=block||frame<cache.frame;
    if(bg_cached&&cache.kind!=kind){cache.bg_ready=0;cache.kind=kind;}
    if(rebuild){
        if(!inflate_counted(cinema_pixels+cinema_offsets[block],cinema_offsets[block+1]-cinema_offsets[block],indexed,cinema_block_sizes[block])){
            for(unsigned i=0;i<256u*192u;i++)pixels[i]=0x8000;
            cinematic_reset();return;
        }
    }
    unsigned start=rebuild?first:cache.frame;
    const uint8_t *patch=indexed+(rebuild?192u*144u:cache.patch);
    for(unsigned f=start+1;f<=frame;f++){
        shift_indexed(indexed,cinema_shifts[f*2],cinema_shifts[f*2+1]);
        const uint8_t *end=patch+cinema_sizes[f];
        while(patch<end){unsigned at=read16(patch),n=read16(patch+2);patch+=4;memcpy(indexed+at,patch,n);patch+=n;}
    }
    if(cached){cache.block=block;cache.frame=frame;cache.patch=(unsigned)(patch-indexed);}
    else for(unsigned y=144;y--;)for(unsigned x=192;x--;)put_pixel(pixels,x,y,indexed[y*192+x]);
    uint8_t rows[CINEMA_BG_MAX_WIDTH*CINEMA_BG_ROWS];
    unsigned loaded=~0u;
    if(full_bg&&!cache.bg_ready){
        for(unsigned row=0;row<(unsigned)(cinema_bg_clip_rows[kind+1]-cinema_bg_clip_rows[kind]);row++){
            unsigned b=cinema_bg_clip_rows[kind]+row;
            uint8_t *out=background+row*width*CINEMA_BG_ROWS;
            if(!inflate_counted(cinema_bg_pixels+cinema_bg_rows[b],cinema_bg_rows[b+1]-cinema_bg_rows[b],out,width*CINEMA_BG_ROWS))memset(out,0,width*CINEMA_BG_ROWS);
        }
        cache.bg_ready=1;
    }
    if(crop_cached&&(!cache.bg_ready||cache.crop!=crop)){
        for(unsigned y=0;y<crop_height;y++){
            unsigned row=(ry+y)/CINEMA_BG_ROWS;
            if(row!=loaded){
                unsigned b=cinema_bg_clip_rows[kind]+row;
                if(!inflate_counted(cinema_bg_pixels+cinema_bg_rows[b],cinema_bg_rows[b+1]-cinema_bg_rows[b],rows,width*CINEMA_BG_ROWS))memset(rows,0,width*CINEMA_BG_ROWS);
                loaded=row;
            }
            memcpy(background+y*crop_width,rows+((ry+y)%CINEMA_BG_ROWS)*width+rx,crop_width);
        }
        cache.bg_ready=1;cache.crop=crop;
    }
    if(bg_cached&&!full_bg&&!crop_cached&&(!cache.bg_ready||cache.cx!=cx||cache.cy!=cy)){
        for(unsigned y=0;y<144;y++){
            unsigned row=(cy+y)/CINEMA_BG_ROWS;
            if(row!=loaded){
                unsigned b=cinema_bg_clip_rows[kind]+row;
                if(!inflate_counted(cinema_bg_pixels+cinema_bg_rows[b],cinema_bg_rows[b+1]-cinema_bg_rows[b],rows,width*CINEMA_BG_ROWS))memset(rows,0,width*CINEMA_BG_ROWS);
                loaded=row;
            }
            memcpy(background+y*192,rows+((cy+y)%CINEMA_BG_ROWS)*width+cx,192);
        }
        cache.bg_ready=1;cache.cx=cx;cache.cy=cy;
    }
    loaded=~0u;
    for(unsigned y=0;y<144;y++){
        unsigned row=(cy+y)/CINEMA_BG_ROWS;
        if(!bg_cached&&row!=loaded){
            unsigned b=cinema_bg_clip_rows[kind]+row;
            if(!inflate_counted(cinema_bg_pixels+cinema_bg_rows[b],cinema_bg_rows[b+1]-cinema_bg_rows[b],rows,width*CINEMA_BG_ROWS))memset(rows,0,width*CINEMA_BG_ROWS);
            loaded=row;
        }
        const uint8_t *line=bg_cached?(crop_cached?background+(cy+y-ry)*crop_width+cx-rx:full_bg?background+(cy+y)*width+cx:background+y*192):rows+((cy+y)%CINEMA_BG_ROWS)*width+cx;
        for(unsigned x=0;x<192;x++){
            unsigned index=cached?indexed[y*192+x]:pixels[third(y*4+1)*256+third(x*4+1)];
            uint16_t color=cinema_bg_palette[kind*128+line[x]];
            if(index){
                unsigned a=cinema_alpha[kind*256+index];uint16_t fore=cinema_palette[kind*256+index];
                if(a==255)color=fore;
                else{
                    unsigned result=0x8000;
                    for(unsigned shift=0;shift<15;shift+=5)result|=div255(((fore>>shift)&31)*a+((color>>shift)&31)*(255-a)+127)<<shift;
                    color=(uint16_t)result;
                }
            }
            if(brightness!=255){
                unsigned faded=0x8000;
                for(unsigned shift=0;shift<15;shift+=5)faded|=div255((((unsigned)color>>shift)&31)*brightness+127)<<shift;
                color=(uint16_t)faded;
            }
            put_pixel(pixels,x,y,color);
        }
    }
}
