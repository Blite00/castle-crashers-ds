#include "asset_inflate.h"
#include <stdio.h>
#include <assert.h>
static uint8_t input[150000],output[150002],expected[150000];
int main(void){FILE*f=fopen("build/inflate-cases.bin","rb");assert(f);unsigned inlen,outlen,count=0;while(fread(&inlen,4,1,f)==1){assert(fread(&outlen,4,1,f)==1);assert(fread(input,1,inlen,f)==inlen);assert(fread(expected,1,outlen,f)==outlen);output[0]=17;output[outlen+1]=29;assert(asset_inflate(input,inlen,output+1,outlen));assert(!memcmp(output+1,expected,outlen));assert(output[0]==17&&output[outlen+1]==29);if(outlen)assert(!asset_inflate(input,inlen,output+1,outlen-1));for(unsigned cut=1;cut<=10&&cut<=inlen;cut++)assert(!asset_inflate(input,inlen-cut,output+1,outlen));count++;}printf("%u dynamic/fixed/stored compression cases match; output bounds and truncation checks pass\n",count);return 0;}
