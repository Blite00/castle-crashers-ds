/* Bounded raw DEFLATE decoding for generated indexed assets. No allocation;
   backreferences read the caller's existing output buffer. */
#ifndef ASSET_INFLATE_H
#define ASSET_INFLATE_H
#include <stdint.h>
#include <string.h>
typedef struct {const uint8_t *p,*end;uint32_t value;unsigned bits;int bad;} AssetBits;
typedef struct {uint16_t count[16];uint16_t *symbols;} AssetHuffman;
static inline unsigned asset_bits(AssetBits *b,unsigned n){
 while(b->bits<n){if(b->p==b->end){b->bad=1;return 0;}b->value|=(uint32_t)*b->p++<<b->bits;b->bits+=8;}
 unsigned v=b->value&((1u<<n)-1);b->value>>=n;b->bits-=n;return v;
}
static inline int asset_tree(AssetHuffman *h,const uint8_t *lengths,unsigned n,uint16_t *fast){
 unsigned next[16]={0},offset[16]={0};int left=1;memset(h->count,0,sizeof(h->count));
 if(fast)memset(fast,0,512*sizeof(*fast));
 for(unsigned i=0;i<n;i++){if(lengths[i]>15)return 0;h->count[lengths[i]]++;}
 h->count[0]=0;
 for(unsigned len=1;len<16;len++){left=(left<<1)-h->count[len];if(left<0)return 0;offset[len]=len==1?0:offset[len-1]+h->count[len-1];next[len]=(next[len-1]+h->count[len-1])<<1;}
 for(unsigned i=0;i<n;i++)if(lengths[i]){unsigned len=lengths[i],code=next[len]++;h->symbols[offset[len]++]=(uint16_t)i;
  if(fast&&len<=9){unsigned reverse=0;for(unsigned k=0;k<len;k++){reverse=(reverse<<1)|(code&1);code>>=1;}
   for(unsigned k=reverse;k<512;k+=1u<<len)fast[k]=(uint16_t)(((i+1)<<4)|len);
  }
 }
 return 1;
}
static inline int asset_symbol(AssetBits *b,const AssetHuffman *h,const uint16_t *fast){
 if(fast){while(b->bits<9&&b->p<b->end){b->value|=(uint32_t)*b->p++<<b->bits;b->bits+=8;}
  unsigned entry=fast[b->value&511],len=entry&15;
  if(len&&len<=b->bits){b->value>>=len;b->bits-=len;return (int)(entry>>4)-1;}
 }
 unsigned code=0,first=0,index=0;
 for(unsigned len=1;len<16;len++){code|=asset_bits(b,1);if(b->bad)return -1;
  unsigned count=h->count[len];if(code>=first&&code<first+count)return h->symbols[index+code-first];
  index+=count;first=(first+count)<<1;code<<=1;
 }
 return -1;
}
static inline int asset_inflate(const uint8_t *src,unsigned size,uint8_t *out,unsigned capacity){
 static const uint16_t length_base[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
 static const uint8_t length_extra[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
 static const uint16_t distance_base[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
 static const uint8_t distance_extra[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
 if(!src||!out)return 0;
 AssetBits bits={src,src+size,0,0,0};unsigned used=0,final=0;
 uint8_t lengths[320],codes[19];uint16_t lit_symbols[288],dist_symbols[32],code_symbols[19],fast[512];
 AssetHuffman lit={{0},lit_symbols},dist={{0},dist_symbols},code={{0},code_symbols};
 while(!final){final=asset_bits(&bits,1);unsigned type=asset_bits(&bits,2);if(bits.bad)return 0;
  if(type==0){asset_bits(&bits,bits.bits&7);unsigned n=asset_bits(&bits,16),inverse=asset_bits(&bits,16);if(bits.bad||(n^inverse)!=65535||n>capacity-used)return 0;while(n--){out[used++]=(uint8_t)asset_bits(&bits,8);if(bits.bad)return 0;}continue;}
  unsigned nl=288,nd=32;
  if(type==1){for(unsigned i=0;i<288;i++)lengths[i]=i<144?8:i<256?9:i<280?7:8;memset(lengths+288,5,32);}
  else if(type==2){nl=257+asset_bits(&bits,5);nd=1+asset_bits(&bits,5);unsigned nc=4+asset_bits(&bits,4);if(bits.bad||nl>286)return 0;memset(codes,0,sizeof(codes));
   /* RFC1951 code-length permutation. */
   static const uint8_t permutation[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
   for(unsigned i=0;i<nc;i++)codes[permutation[i]]=(uint8_t)asset_bits(&bits,3);
   if(bits.bad||!asset_tree(&code,codes,19,0))return 0;
   unsigned n=0;while(n<nl+nd){int symbol=asset_symbol(&bits,&code,0);if(symbol<0)return 0;
    if(symbol<16)lengths[n++]=(uint8_t)symbol;
    else{unsigned count,value=0;if(symbol==16){if(!n)return 0;value=lengths[n-1];count=3+asset_bits(&bits,2);}else if(symbol==17)count=3+asset_bits(&bits,3);else if(symbol==18)count=11+asset_bits(&bits,7);else return 0;
     if(bits.bad||count>nl+nd-n)return 0;
     while(count--)lengths[n++]=(uint8_t)value;
    }
   }
  }else return 0;
  if(!lengths[256]||!asset_tree(&lit,lengths,nl,fast)||!asset_tree(&dist,lengths+nl,nd,0))return 0;
  for(;;){int symbol=asset_symbol(&bits,&lit,fast);if(symbol<0)return 0;
   if(symbol<256){if(used==capacity)return 0;out[used++]=(uint8_t)symbol;}
   else if(symbol==256)break;
   else{if(symbol>285)return 0;unsigned i=(unsigned)symbol-257,n=length_base[i]+asset_bits(&bits,length_extra[i]);int ds=asset_symbol(&bits,&dist,0);if(ds<0||ds>=30)return 0;unsigned d=distance_base[ds]+asset_bits(&bits,distance_extra[ds]);if(bits.bad||d>used||n>capacity-used)return 0;while(n--){out[used]=out[used-d];used++;}}
  }
 }
 return !bits.bad&&used==capacity&&bits.p==bits.end&&bits.bits<8;
}
#endif
