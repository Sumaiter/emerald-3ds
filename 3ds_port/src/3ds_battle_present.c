#include "3ds_battle_present.h"
#include <math.h>
#include <string.h>

static const CtrBattlePresentFrame *sCurrent;
const CtrBattlePresentFrame *CtrBattlePresent_Current(void) { return sCurrent; }
void CtrBattlePresent_Bind(const CtrBattlePresentFrame *frame) { sCurrent=frame; }

static bool FiniteRange(float v,float lo,float hi) { return isfinite(v) && v>=lo && v<=hi; }
#include "3ds_battle_present_assets.inc"
static bool MapIdValid(unsigned id,unsigned layout)
{
    return (id>>8)<sizeof(sPresentGroupSizes) && (id&255)<sPresentGroupSizes[id>>8]
        && layout>0 && layout<sizeof(sPresentLayoutSizes)/sizeof(sPresentLayoutSizes[0]);
}
bool CtrBattlePresent_WorldValid(const CtrPresentWorld *w)
{
    if(w->ready>1 || w->shadowCount>4 || w->weather>7 || w->mapCount>CTR_PRESENT_MAPS
       || w->borderCount>CTR_PRESENT_MAPS || w->underground>1 || w->connected>1 || w->outdoor>1) return false;
    unsigned area=(unsigned)w->gridWidth*w->gridHeight;
    if(w->gridWidth>1024 || w->gridHeight>1024 || area>CTR_PRESENT_MAP_CELLS) return false;
    for(unsigned i=0;i<w->mapCount;++i) {
        const CtrPresentMap *m=&w->maps[i];
        if(!MapIdValid(m->id,m->layout) || m->x < -4096 || m->x>4096
           || m->y < -4096 || m->y>4096 || m->lift < -512 || m->lift>512) return false;
    }
    for(unsigned i=0;i<w->borderCount;++i) {
        const CtrPresentBorder *b=&w->borders[i];
        if(!MapIdValid(b->id,b->layout) || b->x < -4096 || b->x>4096
           || b->y < -4096 || b->y>4096 || b->drawable>1) return false;
    }
    if(w->ready) {
        if(!w->mapCount || !w->borderCount || !area || !w->mapHash
           || !MapIdValid((unsigned)w->mapGroup*256+w->mapNum,w->layout)
           || w->maps[0].id!=(unsigned)w->mapGroup*256+w->mapNum
           || w->maps[0].layout!=w->layout || w->maps[0].x || w->maps[0].y
           || w->gridWidth!=sPresentLayoutSizes[w->layout][0]+15u
           || w->gridHeight!=sPresentLayoutSizes[w->layout][1]+14u
           || !FiniteRange(w->camera[8],0.01f,4096) || !FiniteRange(w->camera[9],1,170)) return false;
    }
    return true;
}
bool CtrBattlePresent_Validate(const CtrBattlePresentFrame *p)
{
    if(!p || p->magic!=CTR_PRESENT_MAGIC || !p->frame || (p->flags&~31u)
       || p->lineCount>CTR_PRESENT_LINES || p->lineMask>255 || p->lineRegs>1
       || p->boxHidden>1 || p->boxDecided>1 || p->boxBarsUp>1
       || p->blurStrength>100 || p->borderFollowsEdge>1 || p->measureGlow>1
       || p->boxSlide < -1 || p->boxSlide>600) return false;
    for(unsigned i=0;i<128;++i) if(p->oamExt[i]>16) return false;
    for(unsigned i=0;i<CTR_PRESENT_HUDS;++i) {
        const CtrPresentHud *h=&p->hud[i];
        if(h->used>1 || h->kind>4 || h->gender>2 || h->hpPixels>48 || h->hpColour>2
           || h->expPixels>64 || h->status>5 || h->ball>1 || h->numbers>1
           || h->boxTile>1023 || h->boxTiles>128 || h->barTile>1023 || h->barTiles>9
           || h->boxTile+h->boxTiles>1024 || h->barTile+h->barTiles>1024
           || (h->used && (h->boxTile>988 || !h->boxTiles))
           || !FiniteRange(h->fade,0,1)) return false;
        /* Extended text commands are not nicknames and can advance the
         * normal font reader beyond this fixed-size field. */
        for(unsigned j=0;j<sizeof(h->name);++j) if(h->name[j]==0xfc) return false;
    }
    const CtrPresentWorld *w=&p->world;
    if(!CtrBattlePresent_WorldValid(w)) return false;
    for(unsigned i=0;i<14;++i) if(!FiniteRange(w->camera[i],-8192,8192)) return false;
    for(unsigned i=0;i<5;++i) if(!FiniteRange(w->intro[i],0,i==4?100000000:1)) return false;
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j)
        if(!FiniteRange(w->shadows[i][j],-4096,4096)) return false;
    for(unsigned i=0;i<5;++i) if(!FiniteRange(w->light[i],0,4)) return false;
    for(unsigned i=0;i<3;++i)
        if(!FiniteRange(w->sun[i],-4,4) || !FiniteRange(w->fadeRgb[i],0,1)) return false;
    return FiniteRange(w->weatherAmount,0,1) && FiniteRange(w->lightning,0,1)
        && FiniteRange(w->fog,0,1) && FiniteRange(w->fade,0,1) && FiniteRange(w->bloom,0,4);
}

static unsigned WrapLerp(unsigned a,unsigned b,unsigned mask,unsigned e,unsigned d)
{
    int distance=(int)((b-a+(mask+1)/2)&mask)-(int)((mask+1)/2);
    return (unsigned)((int)a+distance*(int)e/(int)d)&mask;
}
static float Lerp(float a,float b,float t) { return a+(b-a)*t; }
static uint16_t ColourLerp(uint16_t a,uint16_t b,float t)
{
    uint16_t colour=a&0x8000;
    for(unsigned bit=0;bit<15;bit+=5)
        colour|=(uint16_t)Lerp((a>>bit)&31,(b>>bit)&31,t)<<bit;
    return colour;
}
void CtrBattlePresent_Interpolate(const CtrBattlePresentFrame *a,const CtrBattlePresentFrame *b,
                                unsigned e,unsigned d,CtrBattlePresentFrame *out)
{
    /* New discrete state is exposed only at the end of this interval. In
     * particular a reused OAM slot never flies from the old occupant. */
    if(!d || e>=d) { memcpy(out,b,sizeof(*out));return; }
    memcpy(out,a,sizeof(*out));
    float t=(float)e/(float)d;
    for(unsigned i=0;i<128;++i) {
        const uint16_t *x=a->oam+i*4,*y=b->oam+i*4;
        uint16_t *z=out->oam+i*4;
        bool visible=(x[0]&0x300)!=0x200;
        if(!visible || (x[0]&0xff00)!=(y[0]&0xff00) || (x[1]&0xfe00)!=(y[1]&0xfe00)
           || x[2]!=y[2] || a->oamExt[i]!=b->oamExt[i]) continue;
        int dx=(int)(((y[1]&511)-(x[1]&511)+256)&511)-256;
        int dy=(int)(((y[0]&255)-(x[0]&255)+128)&255)-128;
        /* A discontinuity is a teleport/slot reassignment, not travel. */
        if(dx<-64 || dx>64 || dy<-64 || dy>64) continue;
        z[0]=(x[0]&0xff00)|WrapLerp(x[0]&255,y[0]&255,255,e,d);
        z[1]=(x[1]&0xfe00)|WrapLerp(x[1]&511,y[1]&511,511,e,d);
        if((a->anchored[i/32]&b->anchored[i/32]&(1u<<(i%32)))!=0)
            out->anchor[i]=(int16_t)Lerp(a->anchor[i],b->anchor[i],t);
    }
    for(unsigned bg=0;bg<4;++bg) {
        if(a->regs[4+bg]!=b->regs[4+bg] || a->regs[0]!=b->regs[0]) continue;
        for(unsigned r=0;r<2;++r) {
            unsigned k=8+bg*2+r;
            out->regs[k]=(uint16_t)WrapLerp(a->regs[k]&511,b->regs[k]&511,511,e,d);
            if(a->lineMask==b->lineMask && a->lineCount==b->lineCount)
                for(unsigned line=0;line<a->lineCount;++line)
                    out->scroll[bg*2+r][line]=(uint16_t)WrapLerp(a->scroll[bg*2+r][line]&511,
                                                      b->scroll[bg*2+r][line]&511,511,e,d);
        }
    }
    /* Affine sprite matrices share OAM slots. Interpolate only if all users
     * of the matrix retain their identity; a reassignment stays atomic. */
    for(unsigned m=0;m<32;++m) {
        bool used=false,safe=true;
        for(unsigned i=0;i<128;++i) {
            const uint16_t *x=a->oam+i*4,*y=b->oam+i*4;
            if(!(x[0]&0x100) || ((x[1]>>9)&31)!=m) continue;
            used=true;
            if((x[0]&0xff00)!=(y[0]&0xff00) || (x[1]&0xfe00)!=(y[1]&0xfe00) || x[2]!=y[2]) safe=false;
        }
        if(used && safe) for(unsigned j=0;j<4;++j) {
            unsigned k=m*16+j*4+3;
            out->oam[k]=(uint16_t)(int16_t)Lerp((int16_t)a->oam[k],(int16_t)b->oam[k],t);
        }
    }
    if(!memcmp(a->vram,b->vram,sizeof(a->vram))) {
        for(unsigned i=0;i<512;++i) out->palette[i]=ColourLerp(a->palette[i],b->palette[i],t);
        for(unsigned i=0;i<256;++i) out->extPalette[i]=ColourLerp(a->extPalette[i],b->extPalette[i],t);
    }
    if(a->regs[40]==b->regs[40]) {
        out->regs[41]=(uint16_t)Lerp(a->regs[41]&31,b->regs[41]&31,t)
            | (uint16_t)Lerp((a->regs[41]>>8)&31,(b->regs[41]>>8)&31,t)<<8;
        out->regs[42]=(uint16_t)Lerp(a->regs[42]&31,b->regs[42]&31,t);
    }
    for(unsigned i=0;i<CTR_PRESENT_HUDS;++i) {
        const CtrPresentHud *x=&a->hud[i],*y=&b->hud[i];
        CtrPresentHud *z=&out->hud[i];
        if(!x->used || !y->used || x->kind!=y->kind || x->boxTile!=y->boxTile
           || memcmp(x->name,y->name,sizeof(x->name))) continue;
        z->hp=(int16_t)Lerp(x->hp,y->hp,t);
        z->hpPixels=(uint8_t)Lerp(x->hpPixels,y->hpPixels,t);
        z->expPixels=(uint8_t)Lerp(x->expPixels,y->expPixels,t);
        if(x->fadeColour==y->fadeColour) z->fade=Lerp(x->fade,y->fade,t);
    }
    if(a->world.ready && b->world.ready && a->world.mapHash==b->world.mapHash
       && a->world.layout==b->world.layout && a->world.mapGroup==b->world.mapGroup
       && a->world.mapNum==b->world.mapNum) {
        for(unsigned i=0;i<14;++i) out->world.camera[i]=Lerp(a->world.camera[i],b->world.camera[i],t);
        for(unsigned i=0;i<5;++i) out->world.intro[i]=Lerp(a->world.intro[i],b->world.intro[i],t);
        if(a->world.shadowCount==b->world.shadowCount)
            for(unsigned i=0;i<a->world.shadowCount;++i) for(unsigned j=0;j<4;++j)
                out->world.shadows[i][j]=Lerp(a->world.shadows[i][j],b->world.shadows[i][j],t);
    }
}

uint32_t CtrBattlePresent_Checksum(const uint8_t *data,size_t bytes)
{
    uint32_t hash=2166136261u;
    while(bytes--) hash=(hash^*data++)*16777619u;
    return hash;
}
static uint8_t Delta(const uint8_t *p,const uint8_t *base,size_t at) { return p[at]^(base?base[at]:0); }
static uint32_t sMatch[4096];
static uint32_t DeltaWord(const uint8_t *p,const uint8_t *base,size_t at)
{
    return Delta(p,base,at) | (uint32_t)Delta(p,base,at+1)<<8
        | (uint32_t)Delta(p,base,at+2)<<16 | (uint32_t)Delta(p,base,at+3)<<24;
}
static bool Length(uint8_t *out,size_t cap,size_t *used,size_t n)
{
    while(n>=255) { if(*used==cap) return false;out[(*used)++]=255;n-=255; }
    if(*used==cap) return false;
    out[(*used)++]=(uint8_t)n;return true;
}
size_t CtrBattlePresent_Encode(const uint8_t *p,const uint8_t *base,size_t count,uint8_t *out,size_t cap)
{
    size_t at=0,anchor=0,used=0;
    if(!p || !out || !count || count>CTR_PRESENT_BYTES) return 0;
    memset(sMatch,0xff,sizeof(sMatch));
    /* LZ4-style literal/match lengths. Compress the XOR stream directly,
     * including repetitions across rows, without another full-frame buffer. */
    while(at+4<=count)
    {
        uint32_t word=DeltaWord(p,base,at),hash=(word*2654435761u)>>20;
        uint32_t match=sMatch[hash];sMatch[hash]=(uint32_t)at;
        if(match==UINT32_MAX || at-match>65535 || DeltaWord(p,base,match)!=word) { ++at;continue; }
        size_t run=4,literals=at-anchor;
        while(at+run<count && Delta(p,base,at+run)==Delta(p,base,match+run)) ++run;
        if(used==cap) return 0;
        size_t token=used++;out[token]=(uint8_t)((literals<15?literals:15)<<4);
        if(literals>=15 && !Length(out,cap,&used,literals-15)) return 0;
        if(literals>cap-used) return 0;
        while(anchor<at) { out[used++]=Delta(p,base,anchor);++anchor; }
        if(cap-used<2) return 0;
        unsigned distance=(unsigned)(at-match);
        out[used++]=(uint8_t)distance;out[used++]=(uint8_t)(distance>>8);
        out[token]|=(uint8_t)(run-4<15?run-4:15);
        if(run-4>=15 && !Length(out,cap,&used,run-19)) return 0;
        at+=run;anchor=at;
        if(at>=2 && at+2<=count)
        {
            word=DeltaWord(p,base,at-2);sMatch[(word*2654435761u)>>20]=(uint32_t)(at-2);
        }
    }
    if(anchor<count)
    {
        size_t literals=count-anchor;
        if(used==cap) return 0;
        out[used++]=(uint8_t)((literals<15?literals:15)<<4);
        if(literals>=15 && !Length(out,cap,&used,literals-15)) return 0;
        if(literals>cap-used) return 0;
        while(anchor<count) { out[used++]=Delta(p,base,anchor);++anchor; }
    }
    return used;
}
bool CtrBattlePresent_Decode(const uint8_t *in,size_t size,const uint8_t *base,uint8_t *out,size_t count)
{
    size_t at=0,used=0;
    if(!in || !out || count>CTR_PRESENT_BYTES) return false;
    while(used<size)
    {
        uint8_t token=in[used++],n;
        size_t literals=token>>4,run=(token&15u)+4u;
        if(literals==15) do {
            if(used==size) return false;
            n=in[used++];literals+=n;
            if(literals>count-at) return false;
        } while(n==255);
        if(literals>count-at || literals>size-used) return false;
        while(literals--) { out[at]=in[used++]^(base?base[at]:0);++at; }
        if(used==size) return at==count;
        if(size-used<2) return false;
        unsigned distance=in[used] | (unsigned)in[used+1]<<8;used+=2;
        if(!distance || distance>at) return false;
        if((token&15u)==15) do {
            if(used==size) return false;
            n=in[used++];run+=n;
            if(run>count-at) return false;
        } while(n==255);
        if(run>count-at) return false;
        while(run--) {
            out[at]=out[at-distance]^(base?base[at-distance]^base[at]:0);++at;
        }
    }
    return at==count;
}
