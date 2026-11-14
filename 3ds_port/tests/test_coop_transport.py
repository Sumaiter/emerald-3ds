"""Exercise actual queue admission and queue writes, without a radio thread."""
import argparse
from pathlib import Path
import subprocess
from test_coop_bike_hooks import function

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cc', default='gcc')
    args = parser.parse_args()
    source = (ROOT / '3ds_port/src/3ds_local.c').read_text(encoding='utf-8')
    preamble = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "3ds_local.h"
#include "constants/local_link.h"
#define OUT_QUEUE 128
#define NODE_SLOTS 17
#define MSG_MAX 160
#define MSG_DATA 1
#define CTR_LOG_ERROR 0
struct OutMessage { uint16_t seq; uint8_t type; uint16_t len; uint32_t sentMs,firstMs; uint8_t data[MSG_MAX]; };
struct Node { bool present; struct OutMessage out[OUT_QUEUE]; uint16_t outHead,outCount,nextSeq; bool overflowed; };
static struct { bool ready; unsigned state,myId,lock,wake; struct Node nodes[NODE_SLOTS]; } sL;
static unsigned locks,unlocks,wakes;
static void LightLock_Lock(unsigned *lock) { assert(!*lock);*lock=1;++locks; }
static void LightLock_Unlock(unsigned *lock) { assert(*lock);*lock=0;++unlocks; }
static void svcSignalEvent(unsigned handle) { assert(handle==1 && !sL.lock);++wakes; }
static void CtrLog_Write(int category,const char *format,...) { (void)category;(void)format; }
'''
    bodies = '\n'.join(function(source, name) for name in
                       ('QueueMessage','Wake','LocalLink_TrySend','LocalLink_TrySendLowPriority'))
    harness = r'''
int main(void) {
    uint8_t payload[161]; memset(payload,0x5a,sizeof(payload));
    assert(!LocalLink_TrySend(2,payload,160));
    sL.ready=true;sL.state=LOCAL_SESSION_IN;sL.myId=1;sL.nodes[2].present=true;sL.wake=1;
    assert(!LocalLink_TrySend(0,payload,1));assert(!LocalLink_TrySend(1,payload,1));
    assert(!LocalLink_TrySend(3,payload,1));assert(!LocalLink_TrySend(17,payload,1));
    assert(!LocalLink_TrySend(2,NULL,1));assert(!LocalLink_TrySend(2,payload,0));
    assert(!LocalLink_TrySend(2,payload,161));assert(!sL.nodes[2].outCount);
    sL.nodes[2].outHead=127;
    for(unsigned i=0;i<96;++i)assert(LocalLink_TrySendLowPriority(2,payload,160));
    assert(!LocalLink_TrySendLowPriority(2,payload,160));assert(sL.nodes[2].outCount==96);
    for(unsigned i=96;i<128;++i)assert(LocalLink_TrySend(2,payload,160));
    assert(!LocalLink_TrySend(2,payload,160));assert(sL.nodes[2].outCount==128);
    assert(!sL.nodes[2].overflowed && wakes==128);
    for(unsigned i=0;i<128;++i){struct OutMessage *m=&sL.nodes[2].out[(127+i)%128];
        assert(m->seq==i && m->len==160 && m->type==MSG_DATA && !m->sentMs);
        assert(!memcmp(m->data,payload,160));}
    sL.nodes[2].outCount=0;sL.state=LOCAL_SESSION_NONE;
    assert(!LocalLink_TrySend(2,payload,1));assert(!LocalLink_TrySendLowPriority(2,payload,1));
    assert(locks==unlocks && !sL.lock && wakes==128);
    puts("actual transport queue admission, exact payload, wrap, backpressure and 32 gameplay slots passed");
    return 0;
}
'''
    out = ROOT / 'build/coop-live'
    out.mkdir(parents=True, exist_ok=True)
    c = out / 'transport-native.c'
    exe = out / 'transport-native.exe'
    c.write_text(preamble + bodies + harness, encoding='utf-8')
    subprocess.run([args.cc,'-std=c99','-O2','-Wall','-Wextra','-Werror',
                    '-I'+str(ROOT/'3ds_port/include'),'-I'+str(ROOT/'build/upstream/include'),str(c),'-o',str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

if __name__ == '__main__':
    main()
