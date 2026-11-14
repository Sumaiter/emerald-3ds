/* Optional presentation stream, deliberately independent of engine/save state.
 * One frozen frame, bounded sliding credits, no sends from packet handlers.
 * Every packet uses transport backpressure and leaves gameplay queue reserves. */
#include "3ds_coop_spectator.h"
#include "3ds_coop_protocol.h"
#include "3ds_local.h"
#include "3ds_video.h"
#include <stdlib.h>
#include <string.h>

#ifdef CTR_BATTLE_PRESENTATION
#include "3ds_battle_present.h"
#define PAYLOAD_BYTES CTR_PRESENT_BYTES
#define PAYLOAD_MAX CTR_PRESENT_ENCODED_MAX
#define CaptureRequest CtrVideo_PresentRequestCapture
#define CaptureTake CtrVideo_PresentTakeCapture
#define CaptureCancel CtrVideo_PresentCancelCapture
#define Display CtrVideo_PresentDisplay
#define Encode CtrBattlePresent_Encode
#define Decode CtrBattlePresent_Decode
#define CAPTURE_INTERVAL 16
#define PULL_INTERVAL 96
#else
#define PAYLOAD_BYTES CTR_SPECTATOR_PIXELS
#define PAYLOAD_MAX CTR_SPECTATOR_ENCODED_MAX
#define CaptureRequest CtrVideo_SpectatorRequestCapture
#define CaptureTake CtrVideo_SpectatorTakeCapture
#define CaptureCancel CtrVideo_SpectatorCancelCapture
#define Display CtrVideo_SpectatorDisplay
#define Encode CtrCoopSpectator_Encode
#define Decode CtrCoopSpectator_Decode
#define CAPTURE_INTERVAL 80
#define PULL_INTERVAL RETRY_MS
#endif

enum { STATE, REQUEST, ACCEPT, DENY, STOP, END };
enum { CONTROL_SIZE=12, FRAME_SIZE=22, PULL_SIZE=15, CHUNK_HEADER=14, ACK_SIZE=15 };
enum { TICK_MS=16, RETRY_MS=600, STATUS_MS=1000, PEER_STATUS_MS=3500, LEASE_MS=5000,
       CAPTURE_MS=CAPTURE_INTERVAL, PULL_RETRY=128 };

static struct
{
    uint16_t peer;
    bool connected, localBattle, canWatch, peerBattle, havePeerStatus, statusDirty;
    uint32_t localSerial, peerSerial, peerStatusAt, statusAt, tickAt;
    uint32_t stopToken, stopBattle, stopAt;
    bool stopPending;
    uint8_t reply;
    uint32_t replyToken, replyBattle;
    uint32_t lastViewerToken, lastViewerBattle;
} s;
/* These counters survive reset/disconnect; a delayed packet cannot reopen a
 * canceled view or bind itself to a later encounter after a rejoin. */
static uint32_t sStatusSerial, sRequestSerial;
static struct
{
    uint32_t token, battle, frame, base, size, checksum, heardAt, metaAt, nextAt;
    uint32_t pullOffset, pullEnd, acknowledged, capturedAt;
    bool waitingCapture, frameReady, metaSent;
    uint8_t *encoded, *previous;
    const uint8_t *captured;
} tx;
static struct
{
    uint32_t token, battle, frame, base, size, checksum, offset, windowEnd;
    uint32_t latest, latestChecksum, heardAt, requestAt, pullAt, pullOffset, ackAt;
    bool accepted, frameReady, receiving, ackPending, nack;
#ifdef CTR_BATTLE_PRESENTATION
    bool presentPending;
#endif
    uint8_t *encoded, *front, *decode;
} rx;

static uint32_t Read32(const uint8_t *p)
{
    return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static void Write32(uint8_t *p,uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static bool Newer(uint32_t a,uint32_t b) { return a!=b && (uint32_t)(a-b)<0x80000000u; }
static bool Elapsed(uint32_t now,uint32_t then,uint32_t ms) { return (uint32_t)(now-then)>=ms; }
static uint32_t Serial(uint32_t *value) { if (++*value==0) ++*value;return *value; }

uint32_t CtrCoopSpectator_Checksum(const uint8_t *data,size_t bytes)
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
size_t CtrCoopSpectator_Encode(const uint8_t *p,const uint8_t *base,size_t count,uint8_t *out,size_t cap)
{
    size_t at=0,anchor=0,used=0;
    if(!p || !out || !count || count>CTR_SPECTATOR_PIXELS) return 0;
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
bool CtrCoopSpectator_Decode(const uint8_t *in,size_t size,const uint8_t *base,uint8_t *out,size_t count)
{
    size_t at=0,used=0;
    if(!in || !out || count>CTR_SPECTATOR_PIXELS) return false;
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

static bool Send(uint8_t *packet,unsigned length,unsigned *budget)
{
    if(!*budget || !s.connected || !LocalLink_TrySendLowPriority(s.peer,packet,length)) return false;
    --*budget;return true;
}
static bool Control(unsigned op,uint32_t token,uint32_t battle,unsigned *budget)
{
    uint8_t packet[CONTROL_SIZE]={CTR_SPECTATOR_CONTROL,CTR_COOP_PROTOCOL_VERSION,(uint8_t)op,0};
    Write32(packet+4,token);Write32(packet+8,battle);
    return Send(packet,sizeof(packet),budget);
}
static void ClearTx(void)
{
    CaptureCancel();
    free(tx.encoded);free(tx.previous);memset(&tx,0,sizeof(tx));
}
static void ClearRx(void)
{
    Display(NULL);
    free(rx.encoded);free(rx.front);free(rx.decode);memset(&rx,0,sizeof(rx));
}
void CtrCoopSpectator_Reset(void)
{
    ClearTx();ClearRx();memset(&s,0,sizeof(s));
}
void CtrCoopSpectator_Stop(void)
{
    if(rx.token)
    {
        s.stopToken=rx.token;s.stopBattle=rx.battle;s.stopAt=LocalLink_Milliseconds();s.stopPending=true;
    }
    ClearRx();
}
bool CtrCoopSpectator_IsWatching(void) { return rx.token!=0; }
bool CtrCoopSpectator_HasViewer(void) { return tx.token!=0; }
bool CtrCoopSpectator_HasFrame(void) { return rx.token && rx.frameReady; }
bool CtrCoopSpectator_CanWatchPeer(void)
{
    return s.connected && s.canWatch && !s.localBattle && !rx.token && !tx.token
        && s.havePeerStatus && s.peerBattle && !Elapsed(LocalLink_Milliseconds(),s.peerStatusAt,PEER_STATUS_MS);
}
bool CtrCoopSpectator_Request(void)
{
    if(!CtrCoopSpectator_CanWatchPeer()) return false;
    rx.encoded=malloc(PAYLOAD_MAX);
    rx.front=calloc(1,PAYLOAD_BYTES);rx.decode=malloc(PAYLOAD_BYTES);
    if(!rx.encoded || !rx.front || !rx.decode) { ClearRx();return false; }
    rx.token=Serial(&sRequestSerial);rx.battle=s.peerSerial;
    rx.heardAt=LocalLink_Milliseconds();rx.requestAt=rx.heardAt-RETRY_MS;
    return true;
}
bool CtrCoopSpectator_InputGate(uint16_t *held,uint16_t *pressed)
{
    if(!rx.token) return false;
    if(pressed && (*pressed&2u)) CtrCoopSpectator_Stop();
    if(held) *held=0;
    if(pressed) *pressed=0;
    return true;
}
void CtrCoopSpectator_SetState(uint16_t peer,bool connected,bool peerHint,bool localBattle,bool canWatch)
{
    (void)peerHint;
    connected=connected && peer!=0;
    if(s.connected!=connected || (connected && s.peer!=peer))
    {
        CtrCoopSpectator_Reset();s.connected=connected;s.peer=connected?peer:0;
        s.localSerial=Serial(&sStatusSerial);s.statusDirty=true;
    }
    if(s.localBattle!=localBattle)
    {
        s.localBattle=localBattle;s.localSerial=Serial(&sStatusSerial);s.statusDirty=true;
    }
    s.canWatch=canWatch;
    if(rx.token && (!connected || !canWatch || localBattle)) CtrCoopSpectator_Stop();
    if(tx.token && (!connected || !localBattle))
    {
        s.reply=END;s.replyToken=tx.token;s.replyBattle=tx.battle;ClearTx();
    }
}

static void HandleControl(const uint8_t *p,uint32_t now)
{
    uint32_t token=Read32(p+4),battle=Read32(p+8);
    if(p[3] || p[2]>END || !token) return;
    switch(p[2])
    {
    case STATE:
        if(battle && battle!=token) return;
        if(s.havePeerStatus && token!=s.peerSerial && !Newer(token,s.peerSerial)) return;
        if(s.havePeerStatus && token==s.peerSerial && s.peerBattle!=(battle!=0)) return;
        s.havePeerStatus=true;s.peerStatusAt=now;s.peerSerial=token;s.peerBattle=battle!=0;
        if(rx.token && (!battle || rx.battle!=token)) CtrCoopSpectator_Stop();
        break;
    case REQUEST:
        if(!s.localBattle || battle!=s.localSerial || rx.token || (tx.token && tx.token!=token)
         || (!tx.token && s.lastViewerBattle==battle && !Newer(token,s.lastViewerToken)))
        {
            s.reply=DENY;s.replyToken=token;s.replyBattle=battle;break;
        }
        if(!tx.token)
        {
            tx.encoded=malloc(PAYLOAD_MAX);tx.previous=calloc(1,PAYLOAD_BYTES);
            if(!tx.encoded || !tx.previous)
            {
                ClearTx();s.reply=DENY;s.replyToken=token;s.replyBattle=battle;break;
            }
            tx.token=token;tx.battle=battle;tx.waitingCapture=true;tx.nextAt=now;
            s.lastViewerToken=token;s.lastViewerBattle=battle;
        }
        tx.heardAt=now;s.reply=ACCEPT;s.replyToken=token;s.replyBattle=battle;
        break;
    case ACCEPT:
        if(token==rx.token && battle==rx.battle) { rx.accepted=true;rx.heardAt=now; }
        break;
    case DENY:
    case END:
        if(token==rx.token && battle==rx.battle) ClearRx();
        break;
    case STOP:
        if(token==tx.token && battle==tx.battle) ClearTx();
        break;
    }
}
static void HandleFrame(const uint8_t *p,uint32_t now)
{
    uint32_t token=Read32(p+2),frame=Read32(p+6),base=Read32(p+10),size=Read32(p+14),hash=Read32(p+18);
    if(!rx.token || token!=rx.token || !frame || !size || size>PAYLOAD_MAX) return;
    if(frame==rx.latest && hash==rx.latestChecksum)
    {
#ifdef CTR_BATTLE_PRESENTATION
        if(rx.presentPending) { rx.heardAt=now;return; }
#endif
        rx.ackPending=true;rx.nack=false;rx.heardAt=now;return;
    }
#ifdef CTR_BATTLE_PRESENTATION
    if(rx.presentPending) return;
#endif
    if((rx.latest && !Newer(frame,rx.latest)) || (base && base!=rx.latest)) return;
    if(rx.receiving)
    {
        if(frame!=rx.frame || base!=rx.base || size!=rx.size || hash!=rx.checksum) return;
        rx.heardAt=now;return;
    }
    rx.accepted=true;rx.receiving=true;rx.frame=frame;rx.base=base;rx.size=size;rx.checksum=hash;
    rx.offset=rx.windowEnd=rx.pullOffset=0;rx.heardAt=now;rx.pullAt=now-RETRY_MS;rx.ackPending=false;
#ifdef CTR_BATTLE_PRESENTATION
    /* The accepted session grants one bounded initial window. Metadata and
     * the first chunks can share a pump; a lost packet still uses PULL. */
    rx.windowEnd=CTR_SPECTATOR_WINDOW*CTR_SPECTATOR_CHUNK_BYTES;
    if(rx.windowEnd>rx.size) rx.windowEnd=rx.size;
    rx.pullAt=now;
#endif
}
static void HandleChunk(const uint8_t *p,unsigned length,uint32_t now)
{
    uint32_t token=Read32(p+2),frame=Read32(p+6),offset=Read32(p+10);
    if(!rx.receiving || token!=rx.token || frame!=rx.frame || offset!=rx.offset || offset>=rx.size) return;
    unsigned bytes=rx.size-offset;
    if(bytes>CTR_SPECTATOR_CHUNK_BYTES) bytes=CTR_SPECTATOR_CHUNK_BYTES;
    if(length!=CHUNK_HEADER+bytes || offset>=rx.windowEnd) return;
    memcpy(rx.encoded+offset,p+CHUNK_HEADER,bytes);rx.offset+=bytes;rx.heardAt=now;
    if(rx.offset!=rx.size) return;
    rx.receiving=false;rx.ackPending=true;rx.ackAt=now-RETRY_MS;
    bool valid=Decode(rx.encoded,rx.size,rx.base?rx.front:NULL,rx.decode,PAYLOAD_BYTES)
        && CtrCoopSpectator_Checksum(rx.decode,PAYLOAD_BYTES)==rx.checksum;
#ifdef CTR_BATTLE_PRESENTATION
    valid=valid && CtrBattlePresent_Validate((const CtrBattlePresentFrame *)rx.decode);
#endif
    rx.nack=!valid;
    if(valid)
    {
        uint8_t *old=rx.front;rx.front=rx.decode;rx.decode=old;
        rx.latest=rx.frame;rx.latestChecksum=rx.checksum;
#ifdef CTR_BATTLE_PRESENTATION
        rx.presentPending=true;rx.ackPending=false;
#else
        if(!Display(rx.front)) { CtrCoopSpectator_Stop();return; }
#endif
        rx.frameReady=true;
    }
}
bool CtrCoopSpectator_HandlePacket(uint16_t from,const uint8_t *p,uint16_t length)
{
    if(!p || !length || p[0]<CTR_SPECTATOR_CONTROL || p[0]>CTR_SPECTATOR_ACK) return false;
    if(!s.connected || from!=s.peer || length<2 || p[1]!=CTR_COOP_PROTOCOL_VERSION) return true;
    uint32_t now=LocalLink_Milliseconds();
    switch(p[0])
    {
    case CTR_SPECTATOR_CONTROL:if(length==CONTROL_SIZE) HandleControl(p,now);break;
    case CTR_SPECTATOR_FRAME:if(length==FRAME_SIZE) HandleFrame(p,now);break;
    case CTR_SPECTATOR_CHUNK:if(length>=CHUNK_HEADER) HandleChunk(p,length,now);break;
    case CTR_SPECTATOR_PULL:
        if(length==PULL_SIZE && tx.frameReady && Read32(p+2)==tx.token && Read32(p+6)==tx.frame
         && Read32(p+10)<tx.size && Read32(p+10)%CTR_SPECTATOR_CHUNK_BYTES==0
         && (p[14]&127u) && (p[14]&127u)<=CTR_SPECTATOR_WINDOW)
        {
            uint32_t offset=Read32(p+10),end=offset+(p[14]&127u)*CTR_SPECTATOR_CHUNK_BYTES;
            /* Cumulative receive credit. A refill may arrive before older
             * chunks have drained; it must never rewind or duplicate them. */
            if(offset<tx.acknowledged || offset>tx.pullOffset) break;
            tx.acknowledged=offset;
            if(p[14]&PULL_RETRY) tx.pullOffset=offset;
            if(end>tx.size) end=tx.size;
            if(end>tx.pullEnd) tx.pullEnd=end;
            tx.heardAt=now;
        }
        break;
    case CTR_SPECTATOR_ACK:
        if(length==ACK_SIZE && tx.frameReady && Read32(p+2)==tx.token && Read32(p+6)==tx.frame
         && Read32(p+10)==tx.checksum && p[14]<=1)
        {
            if(p[14]) { tx.base=0;memset(tx.previous,0,PAYLOAD_BYTES); }
            else {
#ifndef CTR_BATTLE_PRESENTATION
                memcpy(tx.previous,tx.captured,PAYLOAD_BYTES);
#endif
                tx.base=tx.frame;
            }
            tx.frameReady=false;tx.metaSent=false;tx.pullOffset=tx.pullEnd=tx.acknowledged=0;tx.heardAt=now;
            tx.waitingCapture=true;tx.nextAt=tx.capturedAt+CAPTURE_MS;
        }
        break;
    }
    return true;
}

void CtrCoopSpectator_Tick(void)
{
    uint32_t now=LocalLink_Milliseconds();
    if(!s.connected || !Elapsed(now,s.tickAt,TICK_MS)) return;
    s.tickAt=now;
    unsigned budget=CTR_SPECTATOR_TICK_PACKETS;
#ifdef CTR_BATTLE_PRESENTATION
    /* Display admission is pumped here, never inside packet dispatch. ACK
     * gates the next delta, so accepted states stay ordered in a bounded
     * playback queue. Source display frames may coalesce during congestion. */
    if(rx.presentPending && Display(rx.front)) {
        rx.presentPending=false;rx.ackPending=true;rx.ackAt=now-RETRY_MS;
    }
#endif
    if(s.stopPending)
    {
        if(Elapsed(now,s.stopAt,STATUS_MS) || Control(STOP,s.stopToken,s.stopBattle,&budget)) s.stopPending=false;
    }
    if(s.statusDirty || Elapsed(now,s.statusAt,STATUS_MS))
        if(Control(STATE,s.localSerial,s.localBattle?s.localSerial:0,&budget)) { s.statusDirty=false;s.statusAt=now; }
    if(s.reply && Control(s.reply,s.replyToken,s.replyBattle,&budget)) s.reply=0;
    if(rx.token && Elapsed(now,rx.heardAt,LEASE_MS)) CtrCoopSpectator_Stop();
    if(tx.token && Elapsed(now,tx.heardAt,LEASE_MS))
    {
        s.reply=END;s.replyToken=tx.token;s.replyBattle=tx.battle;ClearTx();
    }
    /* A battle menu may hold the top framebuffer for several seconds. Keep
     * the view lease alive while waiting for a fresh capturable image. */
    if(rx.token && ((!rx.accepted && Elapsed(now,rx.requestAt,RETRY_MS))
                || (rx.accepted && !rx.receiving && Elapsed(now,rx.requestAt,STATUS_MS))))
        if(Control(REQUEST,rx.token,rx.battle,&budget)) rx.requestAt=now;
    if(tx.token && tx.waitingCapture && (int32_t)(now-tx.nextAt)>=0)
    {
        CaptureRequest();
        const uint8_t *frame=CaptureTake();
        if(frame)
        {
            tx.capturedAt=now;
            uint32_t checksum=CtrCoopSpectator_Checksum(frame,PAYLOAD_BYTES);
            /* Static battle menus need no duplicate frame traffic. Keep the
             * capture rate bounded while the request/status leases stay live. */
            if(tx.base && (
#ifdef CTR_BATTLE_PRESENTATION
                !memcmp(frame,tx.previous,4) && !memcmp(frame+8,tx.previous+8,PAYLOAD_BYTES-8)
#else
                checksum==tx.checksum
#endif
                )) { tx.nextAt=now+CAPTURE_MS;return; }
            tx.captured=frame;tx.size=(uint32_t)Encode(frame,tx.base?tx.previous:NULL,
                PAYLOAD_BYTES,tx.encoded,PAYLOAD_MAX);
            if(!tx.size) { s.reply=END;s.replyToken=tx.token;s.replyBattle=tx.battle;ClearTx(); }
            else
            {
                if(++tx.frame==0) ++tx.frame;
                tx.checksum=checksum;
                tx.waitingCapture=false;tx.frameReady=true;tx.metaAt=now-RETRY_MS;
#ifdef CTR_BATTLE_PRESENTATION
                /* Encoding is complete. This is the ACK base, freeing the
                 * capture buffer to follow every source display frame. */
                memcpy(tx.previous,frame,PAYLOAD_BYTES);
                tx.pullEnd=tx.size<CTR_SPECTATOR_WINDOW*CTR_SPECTATOR_CHUNK_BYTES
                    ?tx.size:CTR_SPECTATOR_WINDOW*CTR_SPECTATOR_CHUNK_BYTES;
#endif
            }
        }
    }
    if(tx.frameReady && (!tx.metaSent || Elapsed(now,tx.metaAt,RETRY_MS)))
    {
        uint8_t p[FRAME_SIZE]={CTR_SPECTATOR_FRAME,CTR_COOP_PROTOCOL_VERSION};
        Write32(p+2,tx.token);Write32(p+6,tx.frame);Write32(p+10,tx.base);
        Write32(p+14,tx.size);Write32(p+18,tx.checksum);
        if(Send(p,sizeof(p),&budget)) { tx.metaSent=true;tx.metaAt=now; }
    }
    while(tx.frameReady && tx.metaSent && tx.pullOffset<tx.pullEnd && budget)
    {
        unsigned bytes=tx.size-tx.pullOffset;
        if(bytes>CTR_SPECTATOR_CHUNK_BYTES) bytes=CTR_SPECTATOR_CHUNK_BYTES;
        uint8_t p[CHUNK_HEADER+CTR_SPECTATOR_CHUNK_BYTES]={CTR_SPECTATOR_CHUNK,CTR_COOP_PROTOCOL_VERSION};
        Write32(p+2,tx.token);Write32(p+6,tx.frame);Write32(p+10,tx.pullOffset);
        memcpy(p+CHUNK_HEADER,tx.encoded+tx.pullOffset,bytes);
        if(!Send(p,CHUNK_HEADER+bytes,&budget)) break;
        tx.pullOffset+=bytes;
    }
    if(rx.ackPending && Elapsed(now,rx.ackAt,RETRY_MS))
    {
        uint8_t p[ACK_SIZE]={CTR_SPECTATOR_ACK,CTR_COOP_PROTOCOL_VERSION};
        Write32(p+2,rx.token);Write32(p+6,rx.nack?rx.frame:rx.latest);
        Write32(p+10,rx.nack?rx.checksum:rx.latestChecksum);p[14]=rx.nack;
        if(Send(p,sizeof(p),&budget)) { rx.ackAt=now;rx.ackPending=false; }
    }
    bool retry=Elapsed(now,rx.pullAt,PULL_INTERVAL);
    bool refill=rx.offset!=rx.pullOffset
        && rx.offset+CTR_SPECTATOR_WINDOW/2u*CTR_SPECTATOR_CHUNK_BYTES>=rx.windowEnd;
    if(rx.receiving && (!rx.windowEnd || refill || retry))
    {
        uint8_t p[PULL_SIZE]={CTR_SPECTATOR_PULL,CTR_COOP_PROTOCOL_VERSION};
        Write32(p+2,rx.token);Write32(p+6,rx.frame);Write32(p+10,rx.offset);
        p[14]=CTR_SPECTATOR_WINDOW | (retry && rx.windowEnd?PULL_RETRY:0);
        if(Send(p,sizeof(p),&budget))
        {
            rx.pullAt=now;rx.pullOffset=rx.offset;rx.windowEnd=rx.offset+CTR_SPECTATOR_WINDOW*CTR_SPECTATOR_CHUNK_BYTES;
            if(rx.windowEnd>rx.size) rx.windowEnd=rx.size;
        }
    }
}
