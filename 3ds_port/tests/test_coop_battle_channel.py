"""Actual adapter receive/reassembly: co-op does not open a GBA RFU link."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef bool bool8;
#define TRUE true
#define FALSE false
#define RFU_CHILD_MAX 4
#define LOCAL_ADVERT_SIZE 32
#include "constants/local_link.h"
#define REASON_DISCONNECTED 1
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define TRACE(...) ((void)0)
#define CTR_LOG_ERROR 1
#define CtrLog_Write(...) ((void)0)
struct LocalPeer {u16 id;};
static bool cooperative; static unsigned session=LOCAL_SESSION_IN,kind=LOCAL_KIND_COOP;
static struct LocalPeer peer={42};
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
static unsigned LocalLink_GetSession(void){return session;}
static unsigned LocalLink_GetSessionKind(void){return kind;}
static unsigned LocalLink_GetPeers(struct LocalPeer *out,unsigned max){assert(max==1);*out=peer;return peer.id?1:0;}
static void LocalLink_Send(u16 to,const void *data,unsigned len){(void)to;(void)data;(void)len;}
'''
STUBS = r'''
static void SendPeer(u16 id,u8 type,const void *data,u16 size){(void)id;(void)type;(void)data;(void)size;}
static bool AcceptChild(u16 id){(void)id;return false;}
static void QueueClear(struct FrameQueue *q){memset(q,0,sizeof(*q));}
static void QueuePush(struct FrameQueue *q,const u8 *data,u32 len){(void)q;(void)data;(void)len;}
static void PeerGone(u16 id,u8 reason){(void)id;(void)reason;}
static u8 PlatformRfuPlayerIndexForSlot(u8 slot){return slot+1;}
static void ChannelSendAll(u8,const u8 *,u16,u16);
static void ClearBlockChannel(void);
'''
TEST = r'''
static void deliver(u16 from,const u8 *bytes,unsigned size,u8 type){
    u8 chunk[155];
    for(unsigned offset=0;offset<size;){
        unsigned n=size-offset;if(n>150)n=150;
        chunk[0]=type;chunk[1]=size;chunk[2]=size>>8;chunk[3]=offset;chunk[4]=offset>>8;
        memcpy(chunk+5,bytes+offset,n);HandlePeerMessage(from,chunk,n+5);offset+=n;
    }
}
int main(void){
    (void)LocalLink_GetSession;(void)LocalLink_GetSessionKind;(void)LocalLink_GetPeers;(void)CtrCoop_IsDoubleBattle;
    u8 bytes[520],out[520];for(unsigned i=0;i<sizeof(bytes);++i)bytes[i]=i*17;
    cooperative=true;
    /* Both roles use AD_IDLE, deliberately without any adapter child/host. */
    for(unsigned role=0;role<2;++role){
        sA.state=AD_IDLE;PlatformLinkBattleReset();
        deliver(42,bytes,sizeof(bytes),PEER_BATTLE);
        assert(PlatformLinkBattleRecv(out,sizeof(out))==sizeof(bytes));assert(!memcmp(out,bytes,sizeof(bytes)));
        deliver(41,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);
        deliver(42,bytes,40,PEER_BLOCK);assert(!sBlockQueue.count);
        kind=1;deliver(42,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);kind=LOCAL_KIND_COOP;
        session=0;deliver(42,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);session=LOCAL_SESSION_IN;
        cooperative=false;deliver(42,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);cooperative=true;
    }
    /* Existing native RFU host/child authentication and reassembly still work. */
    cooperative=false;sA.state=AD_HOST;sA.children[0].active=true;sA.children[0].id=42;
    deliver(42,bytes,40,PEER_BATTLE);assert(PlatformLinkBattleRecv(out,sizeof(out))==40);
    deliver(41,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);
    sA.state=AD_CHILD;sA.hostId=42;
    deliver(42,bytes,40,PEER_BATTLE);assert(PlatformLinkBattleRecv(out,sizeof(out))==40);
    deliver(41,bytes,40,PEER_BATTLE);assert(!sBattleQueue.count);
    puts("native RFU + authenticated co-op battle channel, multipart commands and rejected strangers passed");return 0;
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT/'3ds_port/src/3ds_rfu_adapter.c')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    a=p.parse_args();s=a.source.read_text(encoding='utf-8')
    declarations=s[s.index('enum\n{'):s.index('static void ChannelReceive')]
    names=['QueuePushMessage','QueueTakeMessage','ChannelSendTo','ChannelSendAll','ChannelReceive',
           'ClearBlockChannel','HandlePeerMessage','PlatformLinkBattleReset','PlatformLinkBattleRecv']
    code=HEADER+declarations+STUBS+'\n'.join(function(s,n) for n in names)+TEST
    # Handler occurs after the channel bodies, so its forward declaration is unnecessary.
    with tempfile.TemporaryDirectory(prefix='coop-channel-') as tmp:
        src=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe';src.write_text(code,encoding='utf-8')
        subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-iquote',str(UPSTREAM/'include'),str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)

if __name__=='__main__':main()
