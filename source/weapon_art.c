#include "weapon_art.h"
#include <stdio.h>
#include <string.h>

typedef struct {const uint32_t *offsets;const uint16_t *sizes;const uint8_t *blob;unsigned count,blob_len;} StreamTable;
typedef struct {
    const uint16_t *common,*fix; /* held: common[304], fix[4][304]; combo/dash: weapon[samples], fix[4][samples] */
    const int8_t *bounds;
    StreamTable table;
    int present;
} Section;
typedef struct {
    uint8_t *buffer;unsigned capacity,length;
    unsigned weapon;int ok;
    Section held,combo,dash;
} Slot;

static uint8_t pack_hero0[WEAPON_PACK_MAX_BYTES] __attribute__((aligned(4)));
static uint8_t pack_hero1[WEAPON_PACK_MAX_BYTES] __attribute__((aligned(4)));
static uint8_t pack_other[4096] __attribute__((aligned(4)));
static Slot slots[WEAPON_ART_SLOTS]={{.buffer=pack_hero0,.capacity=sizeof pack_hero0,.weapon=255},{.buffer=pack_hero1,.capacity=sizeof pack_hero1,.weapon=255},{.buffer=pack_other,.capacity=sizeof pack_other,.weapon=255}};

static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
/* Parse u32 count, u32 blob_len, u32 offsets[n+1], u16 sizes[n] (padded), blob (padded). Returns bytes consumed or 0. */
static unsigned parse_table(StreamTable *t,const uint8_t *p,unsigned room){
    if(room<8)return 0;
    unsigned n=rd32(p),blob=rd32(p+4);
    if(n==0||n>65535)return 0;
    unsigned offsets_bytes=4*(n+1),sizes_bytes=(2*n+3)&~3u,blob_bytes=(blob+3)&~3u;
    unsigned total=8+offsets_bytes+sizes_bytes+blob_bytes;
    if(total>room||blob_bytes<blob)return 0;
    t->count=n;t->blob_len=blob;t->offsets=(const uint32_t *)(const void *)(p+8);t->sizes=(const uint16_t *)(const void *)(p+8+offsets_bytes);t->blob=p+8+offsets_bytes+sizes_bytes;
    for(unsigned i=0;i<=n;i++)if(t->offsets[i]>blob||(i&&t->offsets[i]<t->offsets[i-1]))return 0;
    return total;
}
static int parse_pack(Slot *s,unsigned weapon){
    const uint8_t *p=s->buffer;unsigned len=s->length;
    memset(&s->held,0,sizeof s->held);memset(&s->combo,0,sizeof s->combo);memset(&s->dash,0,sizeof s->dash);
    if(len<40||memcmp(p,"WPK2",4)||rd32(p+4)!=WEAPON_PACK_TREE_HASH||rd32(p+8)!=weapon)return 0;
    unsigned off[3],sec[3];
    for(unsigned i=0;i<3;i++){off[i]=rd32(p+16+4*i);sec[i]=rd32(p+28+4*i);if((off[i]&3)||off[i]>len||sec[i]>len-off[i])return 0;}
    if(sec[0]){
        const uint8_t *q=p+off[0];unsigned room=sec[0],fixed=2*304+2*4*304+4*304;
        if(room<fixed)return 0;
        s->held.common=(const uint16_t *)(const void *)q;s->held.fix=(const uint16_t *)(const void *)(q+2*304);s->held.bounds=(const int8_t *)(const void *)(q+2*304+2*4*304);
        if(!parse_table(&s->held.table,q+fixed,room-fixed))return 0;
        for(unsigned i=0;i<304*5;i++)if(s->held.common[i]>=s->held.table.count)return 0;
        s->held.present=1;
    }
    for(unsigned k=0;k<2;k++){
        Section *x=k?&s->dash:&s->combo;unsigned samples=k?DASH_ATTACK_SAMPLES:DEFAULT_COMBO_SAMPLES;
        const uint8_t *q=p+off[1+k];unsigned room=sec[1+k],fixed=((2*samples+3)&~3u)+((2*4*samples+3)&~3u);
        if(room<fixed)return 0;
        x->common=(const uint16_t *)(const void *)q;x->fix=(const uint16_t *)(const void *)(q+((2*samples+3)&~3u));
        if(!parse_table(&x->table,q+fixed,room-fixed))return 0;
        for(unsigned i=0;i<samples;i++)if(x->common[i]>=x->table.count)return 0;
        for(unsigned i=0;i<4*samples;i++)if(x->fix[i]>=x->table.count)return 0;
        x->present=1;
    }
    return 1;
}
unsigned weapon_art_loaded(unsigned slot){return slot<WEAPON_ART_SLOTS&&slots[slot].ok?slots[slot].weapon:255;}
void weapon_art_reset(void){for(unsigned i=0;i<WEAPON_ART_SLOTS;i++){slots[i].weapon=255;slots[i].ok=0;slots[i].length=0;}}
int weapon_art_load(unsigned slot,unsigned weapon){
    if(slot>=WEAPON_ART_SLOTS||weapon>=WEAPON_PACK_COUNT)return 0;
    Slot *s=&slots[slot];
    if(s->weapon==weapon)return s->ok; /* A failed load is remembered until the weapon changes. */
    s->weapon=weapon;s->ok=0;s->length=0;
    char path[48];
    snprintf(path,sizeof path,WEAPON_PACK_PREFIX "w%u.bin",weapon);
    FILE *f=fopen(path,"rb");if(!f)return 0;
    unsigned got=(unsigned)fread(s->buffer,1,s->capacity,f);int extra=fgetc(f);fclose(f);
    if(extra!=EOF||got==0)return 0;
    s->length=got;
    s->ok=parse_pack(s,weapon);
    return s->ok;
}
static Slot *ready(unsigned slot,unsigned weapon){return weapon_art_load(slot,weapon)?&slots[slot]:NULL;}

/* ---- Huffman stream decoding, shared by every system */
typedef struct {const int16_t (*tree[2])[2];const uint16_t *fast[2];} Codec;
typedef struct {const uint8_t *p,*end;uint32_t bits;unsigned count;int good;} Bits;
#ifdef ARM9
#define WA_FAST __attribute__((target("arm")))
#else
#define WA_FAST
#endif
WA_FAST static unsigned next_byte(Bits *b,const Codec *c,unsigned ctx){
    const int16_t (*tree)[2]=c->tree[ctx];const uint16_t *fast=c->fast[ctx];
    while(b->count<8&&b->p<b->end){b->bits=(b->bits<<8)|*b->p++;b->count+=8;}
    int node=0;
    if(b->count>=8){unsigned code=fast[(b->bits>>(b->count-8))&255];if(code&32768){b->count-=(code>>8)&31;return code&255;}node=(int)code;b->count-=8;}
    while(node>=0){
        if(!b->count){if(b->p>=b->end){b->good=0;return 0;}b->bits=*b->p++;b->count=8;}
        node=tree[node][(b->bits>>--b->count)&1];
    }
    return (unsigned)(-node-1);
}
WA_FAST static int apply_stream(const StreamTable *t,unsigned stream,const Codec *c,uint8_t *out,const ArtFrame *f){
    if(stream>=t->count)return 0;
    Bits b={.p=t->blob+t->offsets[stream],.end=t->blob+t->offsets[stream+1],.good=1};
    unsigned remaining=t->sizes[stream],cursor=0;
    while(remaining){
        unsigned skip=0,shift=0,v;
        do{if(!remaining||shift>14)return 0;v=next_byte(&b,c,0);remaining--;skip|=(v&127)<<shift;shift+=7;}while(v&128);
        if(!remaining)return 0;
        unsigned n=next_byte(&b,c,0);remaining--;cursor+=skip;
        int x=(int)(cursor%192)-96-f->x,y=(int)(cursor/192)-88-f->y;
        if(n>remaining||x<0||y<0||y>=f->h||x+(int)n>f->w)return 0;
        for(unsigned j=0;j<n;j++)out[y*f->w+x+j]=(uint8_t)next_byte(&b,c,1);
        remaining-=n;cursor+=n;
    }
    return b.good;
}
static const Codec held_codec={{held_tree0,held_tree1},{held_fast0,held_fast1}};
static const Codec combo_codec={{combo_tree0,combo_tree1},{combo_fast0,combo_fast1}};
static const Codec dash_codec={{dash_tree0,dash_tree1},{dash_fast0,dash_fast1}};

/* ---- held weapons */
ArtFrame weapon_art_frame(unsigned slot,unsigned id,unsigned weapon){
    ArtFrame f={0,0,0,0,0};
    if(id>=1216)return f;
    f=knight_frames[id];
    if(weapon<WEAPON_PACK_FIRST_HELD)return f;
    Slot *s=ready(slot,weapon);if(!s||!s->held.present){f.w=f.h=0;return f;}
    const int8_t *b=s->held.bounds+4*(id%304);
    f.x=b[0];f.y=b[1];f.w=(uint8_t)b[2];f.h=(uint8_t)b[3];return f;
}
int weapon_art_apply(unsigned slot,unsigned id,unsigned weapon,uint8_t *out){
    if(id>=1216||!out)return 0;
    if(weapon<WEAPON_PACK_FIRST_HELD)return weapon<WEAPON_PACK_COUNT;
    Slot *s=ready(slot,weapon);if(!s||!s->held.present)return 0;
    const ArtFrame *base=&knight_frames[id];ArtFrame f=weapon_art_frame(slot,id,weapon);
    if(!f.w||(unsigned)f.w*f.h>KNIGHT_WEAPON_FRAME_PIXELS_MAX)return 0;
    unsigned dx=base->x-f.x,dy=base->y-f.y;
    if(dx+base->w>f.w||dy+base->h>f.h)return 0;
    for(int row=base->h-1;row>=0;row--)memmove(out+(row+dy)*f.w+dx,out+row*base->w,base->w);
    memset(out,0,dy*f.w);
    for(unsigned row=dy;row<dy+base->h;row++){memset(out+row*f.w,0,dx);memset(out+row*f.w+dx+base->w,0,f.w-dx-base->w);}
    memset(out+(dy+base->h)*f.w,0,(f.h-dy-base->h)*f.w);
    unsigned n=id%304,color=id/304;
    return apply_stream(&s->held.table,s->held.common[n],&held_codec,out,&f)&&apply_stream(&s->held.table,s->held.fix[color*304+n],&held_codec,out,&f);
}

/* ---- heavy3/stomp and dash keyframes: base and colour layers are weapon independent and stay resident */
static const StreamTable combo_shared={combo_shared_offsets,combo_shared_sizes,combo_shared_data,sizeof combo_shared_sizes/sizeof *combo_shared_sizes,sizeof combo_shared_data};
static const StreamTable dash_shared={dash_shared_offsets,dash_shared_sizes,dash_shared_data,sizeof dash_shared_sizes/sizeof *dash_shared_sizes,sizeof dash_shared_data};
static int layered(unsigned slot,unsigned sample,unsigned color,unsigned weapon,uint8_t *out,int dash){
    unsigned samples=dash?DASH_ATTACK_SAMPLES:DEFAULT_COMBO_SAMPLES,max=dash?DASH_ATTACK_PIXELS_MAX:DEFAULT_COMBO_PIXELS_MAX;
    if(sample>=samples||color>=4||!out)return 0;
    Slot *s=ready(slot,weapon);if(!s)return 0;
    const Section *x=dash?&s->dash:&s->combo;
    const ArtFrame *f=dash?&dash_frames[sample]:&combo_frames[sample];
    const StreamTable *shared=dash?&dash_shared:&combo_shared;const Codec *c=dash?&dash_codec:&combo_codec;
    if((unsigned)f->w*f->h>max)return 0;
    memset(out,0,(unsigned)f->w*f->h);
    unsigned base=dash?dash_base[sample]:combo_base[sample],col=dash?dash_color[color][sample]:combo_color[color][sample];
    return apply_stream(shared,base,c,out,f)&&apply_stream(&x->table,x->common[sample],c,out,f)&&apply_stream(shared,col,c,out,f)&&apply_stream(&x->table,x->fix[color*samples+sample],c,out,f);
}
ArtFrame weapon_art_combo_frame(unsigned slot,unsigned sample,unsigned color,unsigned weapon){
    ArtFrame empty={0,0,0,0,0};(void)slot;(void)weapon;
    return sample>=DEFAULT_COMBO_SAMPLES||color>=4||weapon>=WEAPON_PACK_COUNT?empty:combo_frames[sample];
}
int weapon_art_combo_decode(unsigned slot,unsigned sample,unsigned color,unsigned weapon,uint8_t *out){return layered(slot,sample,color,weapon,out,0);}
ArtFrame weapon_art_dash_frame(unsigned slot,unsigned sample,unsigned color,unsigned weapon){
    ArtFrame empty={0,0,0,0,0};(void)slot;(void)weapon;
    return sample>=DASH_ATTACK_SAMPLES||color>=4||weapon>=WEAPON_PACK_COUNT?empty:dash_frames[sample];
}
int weapon_art_dash_decode(unsigned slot,unsigned sample,unsigned color,unsigned weapon,uint8_t *out){return layered(slot,sample,color,weapon,out,1);}

/* ---- icons */
#define ICON_CACHE 8
static struct {unsigned tag;WeaponIcon icon;} icon_cache[ICON_CACHE]; /* tag is weapon+1; 0 is empty */
static unsigned icon_next;
static int icon_failed[WEAPON_ICON_COUNT];
const WeaponIcon *weapon_icon(unsigned weapon){
    if(weapon>=WEAPON_ICON_COUNT)return NULL;
    for(unsigned i=0;i<ICON_CACHE;i++)if(icon_cache[i].tag==weapon+1)return &icon_cache[i].icon;
    if(icon_failed[weapon])return NULL;
    icon_failed[weapon]=1;
    FILE *f=fopen(WEAPON_PACK_PREFIX "icons.bin","rb");if(!f)return NULL;
    uint8_t head[8];uint8_t entry[8];
    if(fread(head,1,8,f)!=8||memcmp(head,"WIC1",4)||rd32(head+4)<=weapon){fclose(f);return NULL;}
    if(fseek(f,8+4*(long)weapon,SEEK_SET)||fread(entry,1,8,f)!=8){fclose(f);return NULL;}
    unsigned start=rd32(entry),end=rd32(entry+4);
    if(end<=start+64||end-start>64+1100||fseek(f,(long)start,SEEK_SET)){fclose(f);return NULL;}
    uint8_t rec[64+1100];unsigned len=end-start;
    if(fread(rec,1,len,f)!=len){fclose(f);return NULL;}
    fclose(f);
    WeaponIcon *icon=&icon_cache[icon_next].icon;
    for(unsigned i=0;i<32;i++)icon->palette[i]=(uint16_t)(rec[2*i]|(rec[2*i+1]<<8));
    unsigned at=64,n=0;
    while(n<1024){
        if(at>=len)return NULL;
        unsigned token=rec[at++],count=(token&127)+1;if(count>1024-n)return NULL;
        if(token&128){if(at>=len)return NULL;memset(icon->pixels+n,rec[at++],count);}
        else{if(count>len-at)return NULL;memcpy(icon->pixels+n,rec+at,count);at+=count;}
        n+=count;
    }
    if(at!=len)return NULL;
    icon_cache[icon_next].tag=weapon+1;icon_failed[weapon]=0;
    const WeaponIcon *r=&icon_cache[icon_next].icon;icon_next=(icon_next+1)%ICON_CACHE;return r;
}
