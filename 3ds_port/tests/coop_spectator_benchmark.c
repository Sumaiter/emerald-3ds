/* Two actual endpoints; reliable delayed worker, bounded queue and competing
 * gameplay occupancy. Asset-derived pixel frames are supplied by Python. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "3ds_coop_spectator.h"
#ifndef CTR_SPECTATOR_TICK_PACKETS
#define CTR_SPECTATOR_TICK_PACKETS CTR_SPECTATOR_WINDOW
#endif
#define DECLARE(P) void P##SetState(uint16_t,bool,bool,bool,bool);bool P##Request(void); \
void P##Reset(void);void P##Tick(void);bool P##HasFrame(void); \
bool P##HandlePacket(uint16_t,const uint8_t*,uint16_t); \
size_t P##Encode(const uint8_t*,const uint8_t*,size_t,uint8_t*,size_t);
DECLARE(A)
DECLARE(B)
struct Message { uint8_t data[160];unsigned length,from,due,ack,serial;bool used,delivered; };
static struct Message messages[512];
static unsigned now,delay,sends[2],out[2],peak,bytes,shown,changed,latency,latencyMax,lastCapture;
static unsigned serial,refusals;
static bool congestion;
static uint8_t *frames,image[CTR_SPECTATOR_PIXELS],capture[CTR_SPECTATOR_PIXELS],front[CTR_SPECTATOR_PIXELS];
static bool asked;
static unsigned Gameplay(void) {return congestion && now%1000<240?80:8;}
static bool Send(unsigned from,const void *data,uint32_t length)
{
    assert(length<=160);
    if(out[from]+Gameplay()>=96) {++refusals;return false;} /* Actual queue minus 32 reserved. */
    assert(++sends[from]<=CTR_SPECTATOR_TICK_PACKETS);
    for(unsigned i=0;i<512;++i) if(!messages[i].used) {
        struct Message *p=&messages[i];
        p->used=true;p->delivered=false;p->from=from;p->length=length;p->serial=++serial;
        memcpy(p->data,data,length);
        /* 20 ms transport-worker cadence, deterministic 0..7 ms jitter.
         * Keep ordered transport delivery, as the real cumulative-seq link. */
        unsigned worker=((now+19)/20)*20;
        /* An 80-packet story/inventory burst needs three 20 ms service
         * rounds at four 1400-byte frames (eight full packets each). Hold
         * video behind that FIFO traffic, then retain its ACK occupancy. */
        if(congestion && worker%1000<60) worker+=60-worker%1000;
        unsigned due=worker+delay+((now/16)%8);
        for(unsigned j=0;j<512;++j)
            if(messages[j].used && messages[j].from==from && messages[j].due>due) due=messages[j].due;
        p->due=due;p->ack=due+delay+30; /* ACK_DELAY_MS=30 in actual transport. */
        ++out[from];if(out[from]+Gameplay()>peak)peak=out[from]+Gameplay();
        bytes+=length;return true;
    }
    assert(!"event queue overflow");return false;
}
bool ASend(uint16_t p,const void *d,uint32_t n) {assert(p==2);return Send(0,d,n);}
bool BSend(uint16_t p,const void *d,uint32_t n) {assert(p==1);return Send(1,d,n);}
uint32_t ANow(void) {return now;} uint32_t BNow(void) {return now;}
bool ACaptureRequest(void) {assert(!"viewer capture");return false;}
bool BCaptureRequest(void) {asked=true;return true;}
const uint8_t *ACaptureTake(void) {return NULL;}
const uint8_t *BCaptureTake(void) {
 if(!asked)return NULL;
 asked=false;memcpy(capture,image,sizeof(capture));lastCapture=now;return capture;
}
void ACaptureCancel(void) {} void BCaptureCancel(void) {asked=false;}
bool ADisplay(const uint8_t *p) {
 if(p) {++shown;if(memcmp(p,front,sizeof(front)))++changed;memcpy(front,p,sizeof(front));
  unsigned age=now-lastCapture;latency+=age;if(age>latencyMax)latencyMax=age;}
 return true;
}
bool BDisplay(const uint8_t *p) {(void)p;return true;}
static void Step(void)
{
    now+=16;
    for(unsigned i=0;i<2;++i) {
        assert(out[i]+Gameplay()<=128);
        if(out[i]+Gameplay()>peak)peak=out[i]+Gameplay();
    }
    /* Reliable delivery preserves insertion order, including reused ring
     * slots: choose the earliest due packet separately for each endpoint. */
    for(unsigned endpoint=0;endpoint<2;++endpoint) for(;;) {
        struct Message *next=NULL;
        for(unsigned i=0;i<512;++i) {
            struct Message *p=&messages[i];
            if(p->used && !p->delivered && p->from==endpoint && p->due<=now
             && (!next || p->due<next->due || (p->due==next->due && p->serial<next->serial)))next=p;
        }
        if(!next)break;
        next->delivered=true;
        assert(endpoint?AHandlePacket(2,next->data,next->length):BHandlePacket(1,next->data,next->length));
    }
    for(unsigned i=0;i<512;++i) if(messages[i].used && messages[i].ack<=now) {
        --out[messages[i].from];messages[i].used=false;
    }
    sends[0]=sends[1]=0;ATick();BTick();
}
static void Run(const char *name,unsigned rtt,bool busy)
{
    AReset();BReset();memset(messages,0,sizeof(messages));memset(out,0,sizeof(out));
    now=10000;delay=rtt/2;congestion=busy;peak=bytes=shown=changed=latency=latencyMax=refusals=0;
    memcpy(image,frames,CTR_SPECTATOR_PIXELS);memset(front,0,sizeof(front));
    ASetState(2,true,false,false,true);BSetState(1,true,false,true,false);
    for(unsigned i=0;i<70;++i)Step();
    assert(ARequest());unsigned start=now;
    while(!AHasFrame() && now-start<60000)Step();
    assert(AHasFrame() && !memcmp(front,frames,CTR_SPECTATOR_PIXELS));
    unsigned first=now-start,animationStart=now;
    changed=shown=latency=latencyMax=bytes=0;
    while(now-animationStart<8000) {
        unsigned f=((now-animationStart)/80)%6;
        memcpy(image,frames+f*CTR_SPECTATOR_PIXELS,CTR_SPECTATOR_PIXELS);Step();
    }
    printf("%s,%u,%u,%u,%.3f,%u,%u,%u,%u,%u\n",name,rtt,busy,first,changed/8.0,
           shown?latency/shown:0,latencyMax,bytes/8,peak,refusals);
    AReset();BReset();
}
int main(int argc,char **argv)
{
    assert(argc==3);
    FILE *f=fopen(argv[1],"rb");assert(f);
    frames=malloc(6*CTR_SPECTATOR_PIXELS);assert(frames);
    assert(fread(frames,CTR_SPECTATOR_PIXELS,6,f)==6);fclose(f);
    uint8_t *encoded=malloc(CTR_SPECTATOR_ENCODED_MAX);assert(encoded);
    size_t key=AEncode(frames,NULL,CTR_SPECTATOR_PIXELS,encoded,CTR_SPECTATOR_ENCODED_MAX),delta=0;
    clock_t start=clock();
    for(unsigned i=0;i<60;++i)
        delta=AEncode(frames+(i%5+1)*CTR_SPECTATOR_PIXELS,frames,CTR_SPECTATOR_PIXELS,encoded,CTR_SPECTATOR_ENCODED_MAX);
    printf("CODEC,%s,%u,%u,%.3f\n",argv[2],(unsigned)key,(unsigned)delta,
           (double)(clock()-start)*1000/CLOCKS_PER_SEC/60);
    free(encoded);
    static const unsigned rtts[]={0,10,20,40,120,200};
    for(unsigned i=0;i<sizeof(rtts)/sizeof(rtts[0]);++i) {Run(argv[2],rtts[i],false);Run(argv[2],rtts[i],true);}
    free(frames);return 0;
}
