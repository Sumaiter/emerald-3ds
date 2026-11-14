"""Execute real capture/upload/overlay bodies and check their GPU fence placement."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "3ds_coop_spectator.h"
#define GPU_RGBA5551 1
#define GPU_RB_RGBA8 2
#define GPU_NEAREST 3
#define GPU_CLAMP_TO_EDGE 4
#define GPU_ALWAYS 5
#define GPU_WRITE_COLOR 6
#define GPU_SCISSOR_DISABLE 7
typedef struct { void *data; } C3D_Tex;
typedef struct { struct { void *colorBuf;unsigned colorFmt,width,height; } frameBuf; } C3D_RenderTarget;
typedef struct { unsigned short width,height;float left,top,right,bottom; } Tex3DS_SubTexture;
typedef struct { C3D_Tex *tex;const Tex3DS_SubTexture *sub; } C2D_Image;
static uint8_t *sSpectatorCapture;
static bool sSpectatorCaptureAsked,sSpectatorCaptureReady,sSpectatorLastBattleFrame;
static const uint8_t *sSpectatorPixels;
static bool sSpectatorDirty;
static C3D_Tex sSpectatorTex;
static C3D_RenderTarget left,right,*sTop=&left,*sTopRight=&right;
static unsigned allocations,filterCalls,draws,target,blendCalls;
static bool failAllocation;
static bool C3D_TexInit(C3D_Tex *tex,unsigned w,unsigned h,unsigned fmt) {
    assert(w==512 && h==256 && fmt==GPU_RGBA5551);++allocations;
    if(failAllocation)return false;
    tex->data=calloc(w*h,2);return tex->data!=NULL;
}
static void C3D_TexSetFilter(C3D_Tex *tex,int min,int mag) {
    assert(tex->data && min==GPU_NEAREST && mag==GPU_NEAREST);++filterCalls;
}
static void C3D_TexSetWrap(C3D_Tex *tex,int a,int b) { assert(tex->data && a==4 && b==4); }
static void C2D_Prepare(void) {}
static void C3D_DepthTest(bool a,int b,int c) { assert(!a && b==5 && c==6); }
static void BlendForget(void) {}
static void Blend(int a,bool b,bool c) { assert(a==5 && !b && !c);++blendCalls; }
static void C3D_SetScissor(int a,int b,int c,int d,int e) { assert(a==7 && !b && !c && !d && !e); }
static void SceneOn(C3D_RenderTarget *t) { target=t==&right?1:0; }
static void C2D_ViewReset(void) {}
static void C2D_DrawImageAt(C2D_Image image,int x,int y,int depth,void *tint,int sx,int sy) {
    assert(image.tex==&sSpectatorTex && image.sub->width==400 && image.sub->height==240);
    assert(image.sub->left==0 && image.sub->top==1 && image.sub->right==400.0f/512);
    assert(!x && !y && !depth && !tint && sx==1 && sy==1);++draws;
}
static void C2D_Flush(void) {}
'''
HARNESS = r'''
int main(void) {
    uint32_t *fb=calloc(400*240,sizeof(*fb));
    uint8_t *indices=malloc(CTR_SPECTATOR_PIXELS);
    assert(fb && indices);
    left.frameBuf.colorBuf=fb;left.frameBuf.colorFmt=GPU_RB_RGBA8;
    left.frameBuf.width=240;left.frameBuf.height=400;
    SpectatorCaptureCompleted();assert(!sSpectatorCaptureReady && !sSpectatorCapture);
    assert(CtrVideo_SpectatorRequestCapture());SpectatorCaptureCompleted();
    assert(!CtrVideo_SpectatorTakeCapture()); /* First battle frame has not completed yet. */
    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<400;x++){
        unsigned r=x%8,g=y%8,b=(x/8+y/8)%4;
        uint32_t rgba=((r*255/7)<<24)|((g*255/7)<<16)|((b*255/3)<<8)|255;
        fb[CtrVideo_Texel(239-y,x,240)]=rgba;
        indices[y*400+x]=(uint8_t)((r<<5)|(g<<2)|b);
    }
    sSpectatorLastBattleFrame=true;SpectatorCaptureCompleted();
    const uint8_t *captured=CtrVideo_SpectatorTakeCapture();
    assert(captured && !memcmp(captured,indices,CTR_SPECTATOR_PIXELS));
    assert(!CtrVideo_SpectatorTakeCapture());
    memset(fb,0,400*240*4);assert(captured[399]!=0); /* Frozen until a new request. */
    assert(CtrVideo_SpectatorDisplay(indices));assert(allocations==1 && filterCalls==1);
    SpectatorUploadCompleted();assert(!sSpectatorDirty);
    uint16_t *texture=sSpectatorTex.data;
    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<400;x++){
        unsigned index=indices[y*400+x];
        unsigned r=(index>>5)*31/7,g=((index>>2)&7)*31/7,b=(index&3)*31/3;
        assert(texture[CtrVideo_Texel(x,y,512)]==((r<<11)|(g<<6)|(b<<1)|1));
    }
    assert(CtrVideo_SpectatorRequestCapture());SpectatorCaptureCompleted();
    assert(!CtrVideo_SpectatorTakeCapture()); /* Never rebroadcast the viewer overlay. */
    RenderSpectator(true);assert(draws==2 && target==1 && blendCalls==1);
    void *retained=sSpectatorTex.data;assert(CtrVideo_SpectatorDisplay(NULL));
    RenderSpectator(true);assert(draws==2 && retained==sSpectatorTex.data);
    assert(CtrVideo_SpectatorDisplay(indices));assert(allocations==1);
    CtrVideo_SpectatorCancelCapture();assert(!sSpectatorCaptureAsked && !sSpectatorCaptureReady);
    assert(!sSpectatorLastBattleFrame);
    CtrVideo_SpectatorDisplay(NULL);free(sSpectatorTex.data);sSpectatorTex.data=NULL;
    failAllocation=true;assert(!CtrVideo_SpectatorDisplay(indices) && !sSpectatorPixels);
    free(sSpectatorCapture);free(indices);free(fb);
    puts("actual spectator capture/quantization/texture/overlay bodies and GPU fence order passed");
    return 0;
}
'''

UI_HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "3ds_locale_text.h"
typedef uint8_t u8;
typedef struct {int r,g,b;} Rgb;
typedef int Font;
typedef struct { bool watchFrame;u8 watchName[8]; } ViewState;
#define W 320
#define H 240
static Font sNormal,sSmall;
static Rgb sCream,sWhite;
static u8 label[40];
static uint32_t PackRgb(int r,int g,int b) {(void)r;(void)g;(void)b;return 0;}
static void FillRect(int x,int y,int w,int h,uint32_t c) {(void)x;(void)y;(void)w;(void)h;(void)c;}
static int SmoothInkWidth(const Font *f,const u8 *p,int scale) {
 (void)f;(void)scale;int n=0;while(*p++!=255)++n;return n*6;
}
static void DrawSmoothStr(const Font *f,const u8 *p,int x,int y,int scale,Rgb a,Rgb b,void *c) {
 (void)f;(void)x;(void)scale;(void)a;(void)b;(void)c;
 if(y==176){unsigned i=0;do {label[i]=p[i];}while(p[i++]!=255);}
}
'''
UI_TEST = r'''
int main(void) {
 ViewState s={0,{255}};u8 expected[40];
 DrawSpectator(&s);assert(label[0]==0xbc && label[1]==0xf0);
 CtrLocale_ToGame(expected,sizeof(expected),"B: Zuschauen verlassen");
 assert(!memcmp(label,expected,sizeof("B: Zuschauen verlassen")));
 for(unsigned i=0;label[i]!=255;++i)assert(label[i]!=0xac);
 s.watchFrame=true;DrawSpectator(&s);
 for(unsigned i=0;label[i]!=255;++i)assert(label[i]!=0xac);
 puts("actual spectator button/locale conversion: readable B label, no question-mark glyphs");
 return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    video = (ROOT / '3ds_port/src/3ds_video.c').read_text(encoding='utf-8')
    present = function(video, 'CtrVideo_Present')
    assert present.index('C3D_FrameBegin(C3D_FRAME_SYNCDRAW)) return;') < present.index(
        'SpectatorCaptureCompleted();') < present.index('if (ClearsRun())')
    assert present.index('SpectatorCaptureCompleted();') < present.index('SpectatorUploadCompleted();')
    assert present.index('RenderSpectator(stereo);') < present.index('if (bottom) BottomTransfer();')
    assert 'C3D_FrameEnd' not in function(video, 'SpectatorCaptureCompleted')
    assert 'Sync' not in function(video, 'SpectatorCaptureCompleted')
    shutdown = function(video, 'CtrVideo_Shutdown')
    assert shutdown.index('C3D_RenderTargetDelete(sTopRight)') < shutdown.index('C3D_TexDelete(&sSpectatorTex)')
    bodies = function((ROOT / '3ds_port/src/3ds_video_decode.c').read_text(), 'CtrVideo_Texel')
    for name in ('CtrVideo_SpectatorRequestCapture', 'CtrVideo_SpectatorTakeCapture',
                 'CtrVideo_SpectatorCancelCapture', 'CtrVideo_SpectatorDisplay',
                 'SpectatorCaptureCompleted', 'SpectatorUploadCompleted', 'RenderSpectator'):
        bodies += function(video, name)
    with tempfile.TemporaryDirectory(prefix='spectator-video-', dir=os.environ.get('TMPDIR')) as folder:
        folder = Path(folder)
        source = folder / 'video.c'
        source.write_text(HEADER + bodies + HARNESS)
        exe = folder / ('video.exe' if os.name == 'nt' else 'video')
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-I' + str(ROOT / '3ds_port/include'), str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
        ui = (ROOT / '3ds_port/src/3ds_bottom_ui.c').read_text(encoding='utf-8')
        source.write_text(UI_HEADER+function(ui, 'Ascii')+function(ui, 'DrawSpectator')+UI_TEST)
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-I' + str(ROOT / '3ds_port/include'), str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
