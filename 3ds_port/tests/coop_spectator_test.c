#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds_coop_spectator.h"
#include "3ds_coop_protocol.h"

#define DECLARE(P) \
void P##SetState(uint16_t,bool,bool,bool,bool); \
bool P##Request(void);void P##Stop(void);void P##Reset(void);void P##Tick(void); \
bool P##IsWatching(void);bool P##HasViewer(void);bool P##HasFrame(void);bool P##CanWatchPeer(void); \
bool P##HandlePacket(uint16_t,const uint8_t*,uint16_t);bool P##InputGate(uint16_t*,uint16_t*); \
size_t P##Encode(const uint8_t*,const uint8_t*,size_t,uint8_t*,size_t); \
bool P##Decode(const uint8_t*,size_t,const uint8_t*,uint8_t*,size_t); \
uint32_t P##Checksum(const uint8_t*,size_t);
DECLARE(A)
DECLARE(B)

struct Packet { unsigned from,length,due;uint8_t data[160]; };
static struct Packet queue[4096],request,meta,chunk,stop,firstPull;
static unsigned queued,now,sends[2],maxSends[2],captures[2],displays[2],cancels[2],nacks;
static bool blocked[2],capturePaused[2],duplicateChunks,corruptChunk;
static unsigned linkDelay,sourceChunks,droppedChunks;
static bool dropOneChunk;
static uint8_t image[2][CTR_SPECTATOR_PIXELS],capture[2][CTR_SPECTATOR_PIXELS],shown[2][CTR_SPECTATOR_PIXELS];
static bool captureAsked[2],showing[2];
/* Engine state is deliberately outside the module; calls must leave it intact. */
static const uint64_t engineCanary=0x4a357913bb66dd22ULL;
static uint64_t engineState=0x4a357913bb66dd22ULL;

static bool Send(unsigned from,const void *bytes,uint32_t length)
{
    if(blocked[from]) return false;
    assert(length<=160 && queued<4096);
    assert(++sends[from]<=CTR_SPECTATOR_TICK_PACKETS);
    if(sends[from]>maxSends[from])maxSends[from]=sends[from];
    struct Packet *p=&queue[queued++];p->from=from;p->length=length;p->due=now+linkDelay;memcpy(p->data,bytes,length);
    assert(p->data[0]>=70 && p->data[0]<=74);
    if(p->data[0]==70 && p->data[2]==1)request=*p;
    if(p->data[0]==70 && p->data[2]==4)stop=*p;
    if(p->data[0]==71)meta=*p;
    if(p->data[0]==73)chunk=*p;
    if(p->data[0]==73)++sourceChunks;
    if(p->data[0]==72 && !firstPull.length)firstPull=*p;
    if(p->data[0]==74 && p->data[14])++nacks;
    return true;
}
bool ASend(uint16_t peer,const void *p,uint32_t len) { assert(peer==2);return Send(0,p,len); }
bool BSend(uint16_t peer,const void *p,uint32_t len) { assert(peer==1);return Send(1,p,len); }
uint32_t ANow(void) { return now; }
uint32_t BNow(void) { return now; }
bool ACaptureRequest(void) { captureAsked[0]=true;return true; }
bool BCaptureRequest(void) { captureAsked[1]=true;return true; }
const uint8_t *ACaptureTake(void) {
    if(!captureAsked[0] || capturePaused[0])return NULL;
    captureAsked[0]=false;++captures[0];
    memcpy(capture[0],image[0],CTR_SPECTATOR_PIXELS);return capture[0];
}
const uint8_t *BCaptureTake(void) {
    if(!captureAsked[1] || capturePaused[1])return NULL;
    captureAsked[1]=false;++captures[1];
    memcpy(capture[1],image[1],CTR_SPECTATOR_PIXELS);return capture[1];
}
void ACaptureCancel(void) { captureAsked[0]=false;++cancels[0]; }
void BCaptureCancel(void) { captureAsked[1]=false;++cancels[1]; }
bool ADisplay(const uint8_t *pixels) {
    showing[0]=pixels!=NULL;if(pixels){memcpy(shown[0],pixels,CTR_SPECTATOR_PIXELS);++displays[0];}return true;
}
bool BDisplay(const uint8_t *pixels) {
    showing[1]=pixels!=NULL;if(pixels){memcpy(shown[1],pixels,CTR_SPECTATOR_PIXELS);++displays[1];}return true;
}
static void Deliver(const struct Packet *p)
{
    bool handled=p->from?AHandlePacket(2,p->data,p->length):BHandlePacket(1,p->data,p->length);
    assert(handled);
}
static void Step(void)
{
    now+=16;sends[0]=sends[1]=0;ATick();BTick();
    unsigned count=queued;queued=0;
    for(unsigned i=0;i<count;i++)
    {
        struct Packet p=queue[i];
        if(p.due>now) {queue[queued++]=p;continue;}
        if(dropOneChunk && p.data[0]==73) {dropOneChunk=false;++droppedChunks;continue;}
        if(corruptChunk && p.data[0]==73 && p.length>14){p.data[p.length-1]^=7;corruptChunk=false;}
        Deliver(&p);
        if(duplicateChunks && p.data[0]==73) Deliver(&p);
    }
    assert(engineState==engineCanary);
}
static void Steps(unsigned count) { while(count--)Step(); }
static void Fresh(void)
{
    AReset();BReset();queued=0;now+=100;
    memset(blocked,0,sizeof(blocked));memset(captures,0,sizeof(captures));memset(displays,0,sizeof(displays));
    memset(capturePaused,0,sizeof(capturePaused));
    corruptChunk=duplicateChunks=false;nacks=0;
    linkDelay=sourceChunks=droppedChunks=0;dropOneChunk=false;memset(&firstPull,0,sizeof(firstPull));
    for(unsigned k=0;k<2;k++)for(unsigned y=0;y<240;y++)for(unsigned x=0;x<400;x++)
        image[k][y*400+x]=(uint8_t)(y<180?((x/16+y/16+k)%16)*13:((x/4+y/5)%3?255:0));
    ASetState(2,true,false,false,true);BSetState(1,true,false,true,false);
    Steps(3);
    assert(ACanWatchPeer() && !BCanWatchPeer());
}
static void FirstImage(void)
{
    for(unsigned i=0;i<2000 && !AHasFrame();i++)Step();
    assert(AHasFrame() && AIsWatching() && BHasViewer() && showing[0]);
    assert(!memcmp(shown[0],image[1],CTR_SPECTATOR_PIXELS));
}
static void Put32(uint8_t *p,uint32_t n) { p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24; }

static void Codec(void)
{
    static uint8_t base[CTR_SPECTATOR_PIXELS],pixels[CTR_SPECTATOR_PIXELS],decoded[CTR_SPECTATOR_PIXELS];
    static uint8_t encoded[CTR_SPECTATOR_ENCODED_MAX];
    uint32_t random=7;
    for(unsigned mode=0;mode<8;mode++)
    {
        for(unsigned i=0;i<CTR_SPECTATOR_PIXELS;i++)
        {
            random=random*1664525u+1013904223u;base[i]=(uint8_t)(random>>24);
            pixels[i]=mode==0?0:mode==1?255:mode==2?(uint8_t)i:mode==3?base[i]:
                mode==4?base[i]^(i%300==0):mode==5?(uint8_t)(i/127):mode==6?(uint8_t)(i/128):(uint8_t)(random>>16);
        }
        for(unsigned delta=0;delta<2;delta++)
        {
            size_t bytes=AEncode(pixels,delta?base:NULL,CTR_SPECTATOR_PIXELS,encoded,sizeof(encoded));
            assert(bytes && bytes<=sizeof(encoded));
            assert(ADecode(encoded,bytes,delta?base:NULL,decoded,CTR_SPECTATOR_PIXELS));
            assert(!memcmp(pixels,decoded,sizeof(pixels)));
            assert(AChecksum(pixels,sizeof(pixels))==AChecksum(decoded,sizeof(decoded)));
            assert(!ADecode(encoded,bytes-1,delta?base:NULL,decoded,CTR_SPECTATOR_PIXELS));
            assert(!AEncode(pixels,delta?base:NULL,CTR_SPECTATOR_PIXELS,encoded,1));
        }
    }
    uint8_t malformed[]={255};assert(!ADecode(malformed,1,NULL,decoded,128));
    uint8_t overrun[]={255,1};assert(!ADecode(overrun,2,NULL,decoded,127));
    uint8_t literal[]={127,1};assert(!ADecode(literal,2,NULL,decoded,128));
    assert(!ADecode(NULL,0,NULL,decoded,128));
    /* Exercise every short length and randomized/delta boundaries, not just
     * compressible full frames. Guard both encoder and decoder output. */
    for(unsigned count=1;count<=1024;++count) {
        for(unsigned i=0;i<count;++i) {
            random=random*1664525u+1013904223u;pixels[i]=(uint8_t)(random>>24);
            if(count%3==0) pixels[i]=(uint8_t)(i%7);
        }
        decoded[count]=0xa5;
        size_t bytes=AEncode(pixels,base,count,encoded,sizeof(encoded));
        assert(bytes && ADecode(encoded,bytes,base,decoded,count));
        assert(!memcmp(pixels,decoded,count) && decoded[count]==0xa5);
        assert(!AEncode(pixels,base,count,encoded,bytes-1));
    }
    uint8_t zeroDistance[]={0x10,1,0,0};assert(!ADecode(zeroDistance,sizeof(zeroDistance),NULL,decoded,16));
    uint8_t earlyDistance[]={0,1,0};assert(!ADecode(earlyDistance,sizeof(earlyDistance),NULL,decoded,16));
    uint8_t longLiteral[]={0xf0,255,255};assert(!ADecode(longLiteral,sizeof(longLiteral),NULL,decoded,16));
    for(unsigned n=0;n<10000;++n) {
        uint8_t noise[512];
        for(unsigned i=0;i<sizeof(noise);++i) {random=random*1664525u+1013904223u;noise[i]=(uint8_t)(random>>24);}
        unsigned count=n%1024,length=(n*37)%sizeof(noise);decoded[count]=0xa5;
        (void)ADecode(noise,length,n&1?base:NULL,decoded,count);
        assert(decoded[count]==0xa5);
    }
}
static void StreamAndStop(void)
{
    Fresh();assert(captures[0]==0 && captures[1]==0);assert(ARequest());assert(!ARequest());
    duplicateChunks=true;Steps(3);assert(!AHasFrame() && !showing[0]);
    struct Packet overlap=meta;overlap.data[18]^=1;Deliver(&overlap); /* Same serial, conflicting metadata. */
    overlap=meta;Put32(overlap.data+6,0x7fffffffu);Deliver(&overlap); /* Different frame during partial receive. */
    overlap=meta;overlap.data[0]=73;overlap.length=15;
    Put32(overlap.data+10,CTR_SPECTATOR_ENCODED_MAX);Deliver(&overlap);
    Put32(overlap.data+10,0);Deliver(&overlap); /* Truncated chunk payload. */
    FirstImage();
    unsigned initialAt=now,displayMark=displays[0];
    /* Only a small actor/text region changes; unchanged scenery becomes XOR runs. */
    for(unsigned y=100;y<120;y++)for(unsigned x=150;x<180;x++)image[1][y*400+x]^=3;
    for(unsigned i=0;i<300 && (displays[0]==displayMark || memcmp(shown[0],image[1],CTR_SPECTATOR_PIXELS));i++)Step();
    assert(!memcmp(shown[0],image[1],CTR_SPECTATOR_PIXELS));
    printf("spectator sparse animation/text update: %u ms; packet ceiling %u/%u per 16ms\n",now-initialAt,maxSends[0],maxSends[1]);
    capturePaused[1]=true;Steps(400);assert(AIsWatching() && BHasViewer() && showing[0]);
    capturePaused[1]=false;
    uint16_t held=0xffff,pressed=2;assert(AInputGate(&held,&pressed));
    assert(!held && !pressed && !AIsWatching() && !showing[0]);
    held=16;pressed=0;assert(!AInputGate(&held,&pressed) && held==16); /* Walk immediately next frame. */
    Steps(3);assert(!BHasViewer());
    unsigned captured=captures[1];Steps(40);assert(captures[1]==captured);
    Deliver(&request);Steps(2);assert(!BHasViewer()); /* Canceled nonce cannot reopen the stream. */
    assert(ARequest());FirstImage();
    BSetState(1,true,false,false,true);Steps(3);
    assert(!AIsWatching() && !AHasFrame() && !BHasViewer() && !showing[0]);
    assert(!ARequest());
}
static void BackpressureAndCancel(void)
{
    Fresh();assert(ARequest());blocked[0]=blocked[1]=true;Steps(20);
    assert(AIsWatching() && !BHasViewer() && !AHasFrame());
    blocked[0]=blocked[1]=false;FirstImage();
    ASetState(2,true,true,false,false);assert(!AIsWatching() && !showing[0]);Steps(4);assert(!BHasViewer());
    ASetState(2,true,true,false,true);assert(ARequest());Steps(4);
    ASetState(2,true,true,true,false);assert(!AIsWatching());Steps(4);assert(!BHasViewer());
    ASetState(2,true,true,false,true);assert(ARequest());FirstImage();
    blocked[0]=blocked[1]=true;Steps(330);
    assert(!AIsWatching() && !BHasViewer() && !showing[0]);
    Fresh();assert(ARequest());Steps(3);assert(!AHasFrame());
    BSetState(1,true,false,false,true);Steps(3);
    assert(!AIsWatching() && !BHasViewer() && !showing[0]); /* End cancels an incomplete frame. */
    Fresh();blocked[1]=true;Steps(230);
    assert(!ACanWatchPeer() && !ARequest()); /* A stale peer hint is never authorization. */
}
static void CorruptionAndRejoin(void)
{
    Fresh();assert(ARequest());corruptChunk=true;FirstImage();assert(nacks>0);
    struct Packet oldMeta=meta,oldChunk=chunk,oldStop=stop,oldRequest=request;
    uint8_t malformed[160]={CTR_SPECTATOR_FRAME,CTR_COOP_PROTOCOL_VERSION};
    memcpy(malformed,meta.data,meta.length);Put32(malformed+14,CTR_SPECTATOR_ENCODED_MAX+1);
    assert(AHandlePacket(2,malformed,22));
    malformed[1]^=1;assert(AHandlePacket(2,malformed,22));
    assert(AHandlePacket(99,meta.data,meta.length));
    uint8_t wrong[]={69,CTR_COOP_PROTOCOL_VERSION};assert(!AHandlePacket(2,wrong,sizeof(wrong)));
    for(unsigned len=0;len<22;len++)AHandlePacket(2,meta.data,len);
    assert(AHasFrame());
    AReset();BReset();queued=0;assert(!AIsWatching() && !showing[0]);
    ASetState(2,true,false,false,true);BSetState(1,true,false,true,false);Steps(3);
    assert(ARequest());Deliver(&oldMeta);Deliver(&oldChunk);Deliver(&oldStop);Deliver(&oldRequest);
    Steps(3);assert(!AHasFrame());FirstImage();
    ASetState(0,false,false,false,true);BSetState(0,false,false,true,false);
    assert(!AIsWatching() && !BHasViewer() && !showing[0]);
    unsigned count=captures[1];Steps(20);assert(captures[1]==count);
}
static void DelayedSlidingWindow(void)
{
    Fresh();uint32_t rng=27;
    for(unsigned i=0;i<CTR_SPECTATOR_PIXELS;++i) {
        rng=rng*1664525u+1013904223u;image[1][i]=(uint8_t)(rng>>24);
    }
    linkDelay=96;assert(ARequest());dropOneChunk=true;
    FirstImage();assert(droppedChunks==1 && sourceChunks>CTR_SPECTATOR_WINDOW*2);
    unsigned sent=sourceChunks;
    Deliver(&firstPull); /* Delayed old cumulative grant cannot rewind. */
    Steps(30);assert(sourceChunks==sent);
    unsigned presented=displays[0];Steps(90);
    assert(displays[0]==presented && sourceChunks==sent); /* Static menus send no repeated video. */
    AStop();assert(!AIsWatching() && !showing[0]);Steps(40);assert(!BHasViewer());

    Fresh();linkDelay=96;assert(ARequest());
    Steps(25); /* Control/meta/data may all still be in the delayed queue. */
    AStop();unsigned count=displays[0];assert(!AIsWatching() && !showing[0]);
    Steps(50);assert(!AIsWatching() && !BHasViewer() && displays[0]==count);
    assert(engineState==engineCanary);
}
static void PacketParserFuzz(void)
{
    Fresh();assert(ARequest());FirstImage();uint32_t rng=71;
    for(unsigned n=0;n<10000;++n) {
        uint8_t noise[160];
        for(unsigned i=0;i<sizeof(noise);++i) {rng=rng*1664525u+1013904223u;noise[i]=(uint8_t)(rng>>24);}
        noise[0]=70+n%5;noise[1]=CTR_COOP_PROTOCOL_VERSION;
        if(n&1) memcpy(noise+2,meta.data+2,8); /* Real token/frame with hostile lengths/offsets. */
        unsigned length=n%161;
        assert(AHandlePacket(2,noise,length)==(length!=0));
        assert(BHandlePacket(1,noise,length)==(length!=0));
        assert(engineState==engineCanary);
    }
    AReset();BReset();
}
int main(void)
{
    Codec();StreamAndStop();BackpressureAndCancel();CorruptionAndRejoin();DelayedSlidingWindow();PacketParserFuzz();
    AReset();BReset();assert(engineState==engineCanary);
    puts("spectator: lossless codec, 10000 decoder + 20000 packet fuzz cases, delayed sliding/retry/stale/cancel/end/backpressure/rejoin passed");
    return 0;
}
